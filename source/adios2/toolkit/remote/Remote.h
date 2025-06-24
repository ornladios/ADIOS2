/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
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

namespace adios2
{
class Remote
{

public:
    Remote(const adios2::HostOptions &hostOptions);
    virtual ~Remote();

    // Talk to local connection manager and ask for an existing or new SSH connection
    // to 'remoteHost'. Return the local port through which one can talk to the remote server
    int LaunchRemoteServerViaConnectionManager(const std::string remoteHost);

    // Talk to local connection manager and ask for a key
    // Return a hex string of a key
    std::string GetKeyFromConnectionManager(const std::string keyID);

    virtual explicit operator bool() const { return false; }

    virtual void Open(const std::string hostname, const int32_t port, const std::string filename,
                      const Mode mode, bool RowMajorOrdering);

    virtual void OpenSimpleFile(const std::string hostname, const int32_t port,
                                const std::string filename);

    virtual void OpenReadSimpleFile(const std::string hostname, const int32_t port,
                                    const std::string filename, std::vector<char> &contents);

    typedef void *GetHandle;

    virtual GetHandle Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID,
                          Dims &Count, Dims &Start, Accuracy &accuracy, void *dest);

    virtual bool WaitForGet(GetHandle handle);

    virtual GetHandle Read(size_t Start, size_t Size, void *Dest);

    virtual void Close();

    size_t m_Size;

private:
    const std::shared_ptr<adios2::HostOptions> m_HostOptions;
};

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_REMOTE_REMOTE_H_ */
