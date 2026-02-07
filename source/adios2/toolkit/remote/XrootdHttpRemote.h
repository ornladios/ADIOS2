/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * XrootdHttpRemote.h - HTTP/HTTPS-based client for XRootD SSI services
 *
 * This provides the same functionality as XrootdRemote but uses HTTP or HTTPS
 * instead of the native XRootD SSI protocol. It connects to an XRootD
 * server running the HTTP-to-SSI bridge handler.
 */

#ifndef ADIOS2_XROOTDHTTPREMOTE_H
#define ADIOS2_XROOTDHTTPREMOTE_H

#include "Remote.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Forward declarations for CURL handles
typedef void CURL;
typedef void CURLM;
struct curl_slist;

namespace adios2
{

class XrootdHttpRemote;

/**
 * @brief Async operation state for a single HTTP GET/POST request.
 */
struct AsyncGet
{
    std::promise<bool> promise;
    std::string errorMsg;
    void *destBuffer = nullptr;
    size_t destSize = 0;
    std::vector<char> responseData;
    CURL *easyHandle = nullptr;
    ::curl_slist *headers = nullptr;
};

/**
 * @brief A fully-configured easy handle waiting to be added to the multi handle.
 */
struct PendingSubmit
{
    CURL *easyHandle;
    AsyncGet *asyncOp;
};

/**
 * @brief Shared CURL multi pool singleton.
 *
 * All XrootdHttpRemote instances submit fully-configured easy handles to
 * this pool. A single worker thread drives all transfers via one curl_multi
 * handle, bounding the total number of concurrent TCP connections.
 */
class CurlMultiPool
{
public:
    static CurlMultiPool &getInstance();

    /** Submit a fully-configured easy handle for async execution. Thread-safe. */
    void Submit(CURL *easyHandle, AsyncGet *asyncOp);

    // Non-copyable, non-movable
    CurlMultiPool(const CurlMultiPool &) = delete;
    CurlMultiPool &operator=(const CurlMultiPool &) = delete;

private:
    CurlMultiPool();
    ~CurlMultiPool();

    void WorkerLoop();
    void ProcessCompletedTransfers();

    CURLM *m_MultiHandle = nullptr;
    std::thread m_WorkerThread;
    std::atomic<bool> m_Running{false};
    std::mutex m_QueueMutex;
    std::condition_variable m_QueueCV;
    std::deque<PendingSubmit> m_PendingQueue;
};

/**
 * @brief HTTP/HTTPS-based remote access to ADIOS data via XRootD HTTP-SSI bridge
 *
 * This class provides the same interface as XrootdRemote but uses HTTP or HTTPS
 * for transport instead of the native XRootD SSI protocol. This enables:
 * - Access through HTTP/HTTPS ports which are commonly allowed through firewalls
 * - Optional TLS encryption for data in transit (when using HTTPS)
 * - Compatibility with NERSC Spin and other Kubernetes deployments
 *   - Use HTTP mode behind Spin Ingress (which terminates TLS)
 *   - Use HTTPS mode for direct connections
 *
 * All instances share a single CurlMultiPool for efficient connection pooling.
 *
 * The server must be running XRootD with the HTTP-to-SSI bridge handler
 * (libadios2_xrootd_http.so) loaded.
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
                  Dims &Start, Accuracy &accuracy, void *dest) override;

    bool WaitForGet(GetHandle handle) override;
    GetHandle Read(size_t Start, size_t Size, void *Dest) override;
    void Close() override;

    void SetUseHttps(bool useHttps) { m_UseHttps = useHttps; }
    bool GetUseHttps() const { return m_UseHttps; }
    void SetConnectTimeout(long seconds) { m_ConnectTimeout = seconds; }
    void SetRequestTimeout(long seconds) { m_RequestTimeout = seconds; }
    void SetCACertPath(const std::string &path) { m_CACertPath = path; }
    void SetVerifySSL(bool verify) { m_VerifySSL = verify; }

private:
    std::string BuildRequestString(const char *VarName, size_t Step, size_t StepCount,
                                   size_t BlockID, const Dims &Count, const Dims &Start);
    std::string UrlEncode(const std::string &str);
    CURL *CreateEasyHandle(AsyncGet *asyncOp, const std::string &url, const std::string &postData);

    std::string m_BaseUrl;
    std::string m_Filename;
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
