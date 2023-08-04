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

    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode);

    void OpenSimpleFile(const std::string hostname, const int32_t port, const std::string filename);

    typedef int GetHandle;

    GetHandle Get(char *VarName, size_t Step, size_t BlockID, Dims &Count, Dims &Start, void *dest);

    bool WaitForGet(GetHandle handle);

    GetHandle Read(size_t Start, size_t Size, void *Dest);

    int64_t m_ID;
    size_t m_Size;

private:
#ifdef ADIOS2_HAVE_SST
    void InitCMData();
    RemoteCommon::Remote_evpath_state ev_state;
    CMConnection m_conn;
    std::mutex m_CMInitMutex;
#endif
    bool m_Active = false;
};

class CManagerSingleton
{
public:
#ifdef ADIOS2_HAVE_SST
    CManager m_cm = NULL;
#endif
    static CManagerSingleton &Instance(bool &first)
    {
        // Since it's a static variable, if the class has already been created,
        // it won't be created again.
        // And it **is** thread-safe in C++11.
        static CManagerSingleton myInstance;
        static bool internal_first = true;
        // Return a reference to our instance.

        first = internal_first;
        internal_first = false;
        return myInstance;
    }

    // delete copy and move constructors and assign operators
    CManagerSingleton(CManagerSingleton const &) = delete;            // Copy construct
    CManagerSingleton(CManagerSingleton &&) = delete;                 // Move construct
    CManagerSingleton &operator=(CManagerSingleton const &) = delete; // Copy assign
    CManagerSingleton &operator=(CManagerSingleton &&) = delete;      // Move assign

    // Any other public methods.

protected:
#ifdef ADIOS2_HAVE_SST
    CManagerSingleton() { m_cm = CManager_create(); }

    ~CManagerSingleton() { CManager_close(m_cm); }
#else
    CManagerSingleton() {}

    ~CManagerSingleton() {}
#endif
    // And any other protected methods.
};

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_REMOTE_REMOTE_H_ */
