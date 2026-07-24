/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_REMOTE_REMOTE_H_
#define ADIOS2_TOOLKIT_REMOTE_REMOTE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <mutex>
#include <string>
#include <vector>
/// \endcond

#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{

struct RemoteSetup
{
    std::string hostName;
    adios2::HostAccessProtocol protocol = HostAccessProtocol::Invalid;
    adios2::XRootDTransferProtocol xrootdTransferProtocol = XRootDTransferProtocol::XRootD;
    adios2::HostConfig *hostConfig = nullptr;
};

class Remote
{

public:
    Remote();
    Remote(const RemoteSetup &remoteSetup);
    virtual ~Remote();

    // Talk to local connection manager and ask for an existing or new SSH connection
    // to 'remoteHost'. Return the local port through which one can talk to the remote server
    int LaunchRemoteServerViaConnectionManager(const std::string remoteHost);

    // Talk to local connection manager and ask for a key
    // Return a hex string of a key
    std::string GetKeyFromConnectionManager(const std::string keyID);

    virtual explicit operator bool() const { return false; }

    virtual void Open(const std::string hostname, const int32_t port, const std::string filename,
                      const Mode mode, bool RowMajorOrdering, const Params &params = Params());

    virtual void OpenSimpleFile(const std::string hostname, const int32_t port,
                                const std::string filename);

    virtual void OpenReadSimpleFile(const std::string hostname, const int32_t port,
                                    const std::string filename, std::vector<char> &contents);

    typedef void *GetHandle;

    // destSize = expected byte size of dest (0 = unknown/unchecked); lets the
    // backend reject a response that would overrun the buffer.
    virtual GetHandle Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID,
                          Dims &Count, Dims &Start, Accuracy &accuracy, void *dest,
                          size_t destSize);

    virtual bool WaitForGet(GetHandle handle);

    struct BatchGetRequest
    {
        const char *VarName;
        size_t Step;
        size_t StepCount;
        size_t BlockID;
        Dims Count;
        Dims Start;
        Accuracy accuracy;
        void *dest;
        size_t destSize = 0; // expected byte size of dest; 0 = unchecked
    };

    // Batch multiple Get requests into a single round-trip.
    // Returns true on success (all data written to dest buffers),
    // false if batching is not supported (caller should fall back to individual Gets).
    virtual bool BatchGet(const std::vector<BatchGetRequest> &requests) { return false; }

    virtual GetHandle Read(size_t Start, size_t Size, void *Dest);

    virtual void Close();

    size_t m_Size;

private:
    const RemoteSetup m_RemoteSetup;
};

std::string ParamsToEncodedString(const adios2::Params &params);
adios2::Params EncodedStringToParams(const std::string &pstr);

/* Make a RemoteSetup object before creating Remote().
   If remoteHost is given, host config will be searched for connection information.
      If not found in config, returned rs.protocol will be HostAccessProtocol::Invalid.
   If remoteHost is "" then environment variables are searched for connection information.
      If not found in environment, returned rs.protocol will be HostAccessProtocol::Invalid
      and rs.hostName will be "localhost".
*/
RemoteSetup GetRemoteSetup(const std::string &remoteHost);

/* Create/Get a Remote object connected to the target described in RemoteSetup */
std::shared_ptr<adios2::Remote> GetRemote(const RemoteSetup &remoteSetup,
                                          const std::string &RemoteFileName,
                                          const adios2::Mode openMode, const bool rowMajorOrdering,
                                          const Params remoteParams);

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_REMOTE_REMOTE_H_ */
