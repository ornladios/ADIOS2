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

    /*
     * Open() and OpenSimpleFile() are synchronous calls that
     * internally return a unique ID of an open file on the server.
     * Barring an explicit Close() operation, that file will remain
     * open until the connection between the client and server closes.
     * Note that because of connection sharing and other network-level
     * considerations, this might not happen upon destruction of the
     * EVPathRemote object.
     */
    void Open(const std::string hostname, const int32_t port, const std::string filename,
              const Mode mode, bool RowMajorOrdering);

    void OpenSimpleFile(const std::string hostname, const int32_t port, const std::string filename);

    /*
     * OpenReadSimpleFile() is a synchronous call that returns the
     * full contents of a remote simple file as a char array.  It does
     * this with a single round-trip to the server and does not leave
     * an open file on the server.
     */
    void OpenReadSimpleFile(const std::string hostname, const int32_t port,
                            const std::string filename, std::vector<char> &contents);

    GetHandle Get(const char *VarName, size_t Step, size_t StepCount, size_t BlockID, Dims &Count,
                  Dims &Start, Accuracy &accuracy, void *dest);

    bool WaitForGet(GetHandle handle);

    GetHandle Read(size_t Start, size_t Size, void *Dest);

    /*
     * EVPathRemote::Close is an active synchronous operation that
     * involves a round-trip to the server, waiting for a response to
     * ensure that the file is closed on the server.  This should
     * likely not be performed in any destructors because of the
     * delays involved.  However server files are also closed after
     * the network connection from which they were opened goes away.
     * This is a passive asynchronous operation that will happen
     * sometime after the EVPathRemote object is destroyed.
     */
    void Close();

    int64_t m_ID;

    std::vector<char> *m_TmpContentVector = nullptr;
#ifdef ADIOS2_HAVE_SST
    std::mutex m_ResponsesMutex;
    std::map<int, EVPathRemoteCommon::ReadResponseMsg>
        m_Responses; // read/get responses to be processed
#endif

private:
#ifdef ADIOS2_HAVE_SST
    void InitCMData();
    EVPathRemoteCommon::Remote_evpath_state ev_state;
    CMConnection m_conn = NULL;
    void ProcessReadResponse(GetHandle handle);
#endif
    bool m_Active = false;
};

#ifdef ADIOS2_HAVE_SST
class CManagerSingleton
{
public:
    static CManagerSingleton &Instance(EVPathRemoteCommon::Remote_evpath_state &ev_state);

    /* Map of hostname : connection/port */
    static std::map<std::string, std::pair<std::shared_ptr<EVPathRemote>, int>> m_EVPathRemotes;
    /* Helper function to manage connections, one per host */
    static std::pair<std::shared_ptr<EVPathRemote>, int>
    MakeEVPathConnection(const std::string &hostName);

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

    ~CManagerSingleton()
    {
        m_EVPathRemotes.clear();
        CManager_close(m_cm);
    }
};
#endif

} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_EVPATHREMOTE_REMOTE_H_ */
