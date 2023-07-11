/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_TOOLKIT_REMOTE_REMOTE_H_
#define ADIOS2_TOOLKIT_REMOTE_REMOTE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include "adios2/common/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_SST
#include "remote_common.h"
#endif

namespace adios2
{
class Remote
{

public:
    profiling::IOChrono m_Profiler; ///< profiles Open, Write/Read, Close

    /**
     * Base constructor that all derived classes pass
     * @param type from derived class
     * @param comm passed to m_Comm
     */
    Remote();

    explicit operator bool() const { return m_Active; }

    void Open(const std::string hostname, const int32_t port,
              const std::string filename, const Mode mode);

    void OpenSimpleFile(const std::string hostname, const int32_t port,
                        const std::string filename);

    typedef int GetHandle;

    GetHandle Get(char *VarName, size_t Step, Dims &Count, Dims &Start,
                  void *dest);

    bool WaitForGet(GetHandle handle);

    GetHandle Read(size_t Start, size_t Size, void *Dest);

    int64_t m_ID;

private:
#ifdef ADIOS2_HAVE_SST
    RemoteCommon::Remote_evpath_state ev_state;
    CManager m_cm;
    CMConnection m_conn;
#endif
    bool m_Active = false;
};

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_REMOTE_REMOTE_H_ */
