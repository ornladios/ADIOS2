/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef ADIOS2_TOOLKIT_REMOTE_EVPATHREMOTE_H_
#define ADIOS2_TOOLKIT_REMOTE_EVPATHREMOTE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <mutex>
#include <string>
#include <vector>
/// \endcond

#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include "Remote.h"
#include "adios2/common/ADIOSConfig.h"

#include "remote_common.h"

namespace adios2
{

class EVPathRemote : public Remote
{

public:
    profiling::IOChrono m_Profiler; ///< profiles Open, Write/Read, Close

    /**
     * Base constructor that all derived classes pass
     * @param type from derived class
     * @param comm passed to m_Comm
     */
    EVPathRemote(const adios2::HostOptions &hostOptions);
    ~EVPathRemote();

    explicit operator bool() const { return m_Active; }

    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode, bool RowMajorOrdering);

    void OpenSimpleFile(const std::string hostname, const int32_t port, const std::string filename);

    GetHandle Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start, void *dest);

    bool WaitForGet(GetHandle handle);

    GetHandle Read(size_t Start, size_t Size, void *Dest);

    int64_t m_ID;

private:
#ifdef ADIOS2_HAVE_SST
    void InitCMData();
    EVPathRemoteCommon::Remote_evpath_state ev_state;
    CMConnection m_conn = NULL;
    std::mutex m_CMInitMutex;
#endif
    bool m_Active = false;
};

#ifdef ADIOS2_HAVE_SST
class CManagerSingleton
{
public:
    static CManagerSingleton &Instance(EVPathRemoteCommon::Remote_evpath_state &ev_state);

private:
    CManager m_cm = NULL;
    EVPathRemoteCommon::Remote_evpath_state internalEvState;
    CManagerSingleton()
    {
        m_cm = CManager_create();
        internalEvState.cm = m_cm;
        RegisterFormats(internalEvState);
        CMfork_comm_thread(internalEvState.cm);
    }

    ~CManagerSingleton() { CManager_close(m_cm); }
};
#endif

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_EVPATHREMOTE_REMOTE_H_ */
