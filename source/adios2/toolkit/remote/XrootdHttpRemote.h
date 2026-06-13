/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_XROOTDHTTPREMOTE_H
#define ADIOS2_XROOTDHTTPREMOTE_H

#include "Remote.h"
#include "RemoteHttpBackend.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include <memory>
#include <string>

namespace adios2
{

/**
 * @brief HTTP/HTTPS-based remote access to ADIOS data via an XRootD HTTP-SSI
 * bridge or a Pelican federation.
 *
 * This class owns URL building (the path-encoded, cache-friendly request
 * grammar) and response framing/parsing.  The HTTP round trip itself is
 * delegated to a RemoteHttpBackend (libcurl by default).
 */
class XrootdHttpRemote : public Remote
{
public:
    profiling::IOChrono m_Profiler;

    XrootdHttpRemote(const adios2::HostOptions &hostOptions);
    ~XrootdHttpRemote();

    explicit operator bool() const override { return m_OpenSuccess; }

    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode, bool RowMajorOrdering, const Params &params = Params()) override;

    GetHandle Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID, Dims &Count,
                  Dims &Start, Accuracy &accuracy, void *dest, size_t destSize) override;

    bool WaitForGet(GetHandle handle) override;
    GetHandle Read(size_t Start, size_t Size, void *Dest) override;

    bool BatchGet(const std::vector<BatchGetRequest> &requests) override;

    void Close() override;

    void SetUseHttps(bool useHttps) { m_UseHttps = useHttps; }
    bool GetUseHttps() const { return m_UseHttps; }
    void SetConnectTimeout(long seconds) { m_ConnectTimeout = seconds; }
    void SetRequestTimeout(long seconds) { m_RequestTimeout = seconds; }
    void SetCACertPath(const std::string &path) { m_CACertPath = path; }
    void SetVerifySSL(bool verify) { m_VerifySSL = verify; }

private:
    /** Path-encoded request builders.  Each produces one URL path segment.
     *  See developer notes for the wire grammar; summary:
     *    file-config:  r0?p<base64url-EP>?    (`_` placeholder if empty)
     *    single get:   g~<base64url-var>~<paramstring>     (`_` if no params)
     *    batch get:    b~N~<v1>~<p1>~…~<vN>~<pN>
     *  Varnames and EngineParams are base64url-encoded (RFC 4648 §5).
     *  Paramstring fields are letter-prefixed and self-delimiting; vector
     *  elements use `,`; outer delimiter is `~`; floats use `.` as decimal. */
    std::string BuildFileConfigSegment();
    std::string BuildPerRequestParamString(size_t Step, size_t StepCount, size_t BlockID,
                                           const Dims &Count, const Dims &Start,
                                           const Accuracy &accuracy);
    std::string BuildSingleGetSegment(const char *VarName, size_t Step, size_t StepCount,
                                      size_t BlockID, const Dims &Count, const Dims &Start,
                                      const Accuracy &accuracy);
    std::string BuildBatchGetSegment(const std::vector<BatchGetRequest> &requests);

    static std::string Base64urlEncode(const std::string &str);

    /** HTTP transport (libcurl by default). */
    std::unique_ptr<RemoteHttpBackend> m_Backend;

    std::string m_BaseUrl;
    std::string m_Filename;
    std::string m_EngineParams;
    /** File id from the reader's metadata, sent for the server's staleness check;
     *  0 = none (legacy file). */
    uint32_t m_FileUUID = 0;
    /** Per-engine `<file-config>` path segment, computed once at Open()
     *  from RMOrder + EngineParams.  Reused on every request. */
    std::string m_FileConfigSegment;
    Mode m_Mode;
    bool m_RowMajorOrdering;
    bool m_OpenSuccess = false;

    bool m_UseHttps = true;
    long m_ConnectTimeout = 30;
    long m_RequestTimeout = 300;
    std::string m_CACertPath;
    bool m_VerifySSL = true;
};

} // end namespace adios2

#endif // ADIOS2_XROOTDHTTPREMOTE_H
