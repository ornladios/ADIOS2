/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */
#include "EVPathRemote.h"
#include "Remote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"
#ifdef _MSC_VER
#define strdup(x) _strdup(x)
#endif

#define ThrowUp(x)                                                                                 \
    helper::Throw<std::invalid_argument>("Core", "Engine", "ThrowUp",                              \
                                         "Non-overridden function " + std::string(x) +             \
                                             " called in Remote")

namespace adios2
{

EVPathRemote::EVPathRemote(const adios2::HostOptions &hostOptions) : Remote(hostOptions) {}

#ifdef ADIOS2_HAVE_SST
EVPathRemote::~EVPathRemote()
{
    if (m_conn)
        CMConnection_close(m_conn);
}

void OpenResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                         attr_list attrs)
{
    EVPathRemoteCommon::OpenResponseMsg open_response_msg =
        static_cast<EVPathRemoteCommon::OpenResponseMsg>(vevent);

    void *obj = CMCondition_get_client_data(cm, open_response_msg->OpenResponseCondition);
    static_cast<EVPathRemote *>(obj)->m_ID = open_response_msg->FileHandle;
    CMCondition_signal(cm, open_response_msg->OpenResponseCondition);
    return;
};

void OpenSimpleResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                               attr_list attrs)
{
    EVPathRemoteCommon::OpenSimpleResponseMsg open_response_msg =
        static_cast<EVPathRemoteCommon::OpenSimpleResponseMsg>(vevent);

    void *obj = CMCondition_get_client_data(cm, open_response_msg->OpenResponseCondition);
    static_cast<EVPathRemote *>(obj)->m_ID = open_response_msg->FileHandle;
    static_cast<EVPathRemote *>(obj)->m_Size = open_response_msg->FileSize;
    CMCondition_signal(cm, open_response_msg->OpenResponseCondition);
    return;
};

void ReadResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                         attr_list attrs)
{
    EVPathRemoteCommon::ReadResponseMsg read_response_msg =
        static_cast<EVPathRemoteCommon::ReadResponseMsg>(vevent);
    memcpy(read_response_msg->Dest, read_response_msg->ReadData, read_response_msg->Size);
    CMCondition_signal(cm, read_response_msg->ReadResponseCondition);
    return;
};

CManagerSingleton &CManagerSingleton::Instance(EVPathRemoteCommon::Remote_evpath_state &ev_state)
{
    std::mutex mtx;
    const std::lock_guard<std::mutex> lock(mtx);
    static CManagerSingleton instance;
    ev_state = instance.internalEvState;
    return instance;
}

void EVPathRemote::InitCMData()
{
    (void)CManagerSingleton::Instance(ev_state);
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        CMregister_handler(ev_state.OpenResponseFormat, (CMHandlerFunc)OpenResponseHandler,
                           &ev_state);
        CMregister_handler(ev_state.ReadResponseFormat, (CMHandlerFunc)ReadResponseHandler,
                           &ev_state);
        CMregister_handler(ev_state.OpenSimpleResponseFormat,
                           (CMHandlerFunc)OpenSimpleResponseHandler, &ev_state);
        CMregister_handler(ev_state.ReadResponseFormat, (CMHandlerFunc)ReadResponseHandler,
                           &ev_state);
    });
}

void EVPathRemote::Open(const std::string hostname, const int32_t port, const std::string filename,
                        const Mode mode, bool RowMajorOrdering)
{

    EVPathRemoteCommon::_OpenFileMsg open_msg;
    InitCMData();
    attr_list contact_list = create_attr_list();
    atom_t CM_IP_PORT = -1;
    atom_t CM_IP_HOSTNAME = -1;
    CM_IP_HOSTNAME = attr_atom_from_string("IP_HOST");
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    add_attr(contact_list, CM_IP_HOSTNAME, Attr_String, (attr_value)strdup(hostname.c_str()));
    add_attr(contact_list, CM_IP_PORT, Attr_Int4, (attr_value)port);
    m_conn = CMinitiate_conn(ev_state.cm, contact_list);
    free_attr_list(contact_list);
    if (!m_conn)
        return;

    memset(&open_msg, 0, sizeof(open_msg));
    open_msg.FileName = (char *)filename.c_str();
    switch (mode)
    {
    case Mode::Read:
        open_msg.Mode = EVPathRemoteCommon::RemoteFileMode::RemoteOpen;
        break;
    case Mode::ReadRandomAccess:
        open_msg.Mode = EVPathRemoteCommon::RemoteFileMode::RemoteOpenRandomAccess;
        break;
    default:
        break;
    }
    open_msg.OpenResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    open_msg.RowMajorOrder = RowMajorOrdering;
    CMCondition_set_client_data(ev_state.cm, open_msg.OpenResponseCondition, (void *)this);
    CMwrite(m_conn, ev_state.OpenFileFormat, &open_msg);
    CMCondition_wait(ev_state.cm, open_msg.OpenResponseCondition);
    m_Active = true;
}

void EVPathRemote::OpenSimpleFile(const std::string hostname, const int32_t port,
                                  const std::string filename)
{

    EVPathRemoteCommon::_OpenSimpleFileMsg open_msg;
    InitCMData();
    attr_list contact_list = create_attr_list();
    atom_t CM_IP_PORT = -1;
    atom_t CM_IP_HOSTNAME = -1;
    CM_IP_HOSTNAME = attr_atom_from_string("IP_HOST");
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    add_attr(contact_list, CM_IP_HOSTNAME, Attr_String, (attr_value)strdup(hostname.c_str()));
    add_attr(contact_list, CM_IP_PORT, Attr_Int4, (attr_value)port);
    m_conn = CMinitiate_conn(ev_state.cm, contact_list);
    free_attr_list(contact_list);
    if (!m_conn)
        return;

    memset(&open_msg, 0, sizeof(open_msg));
    open_msg.FileName = (char *)filename.c_str();
    open_msg.OpenResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    CMCondition_set_client_data(ev_state.cm, open_msg.OpenResponseCondition, (void *)this);
    CMwrite(m_conn, ev_state.OpenSimpleFileFormat, &open_msg);
    CMCondition_wait(ev_state.cm, open_msg.OpenResponseCondition);
    m_Active = true;
}

EVPathRemote::GetHandle EVPathRemote::Get(char *VarName, size_t Step, size_t BlockID, Dims &Count,
                                          Dims &Start, void *dest)
{
    EVPathRemoteCommon::_GetRequestMsg GetMsg;
    memset(&GetMsg, 0, sizeof(GetMsg));
    GetMsg.GetResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    GetMsg.FileHandle = m_ID;
    GetMsg.VarName = VarName;
    GetMsg.Step = Step;
    GetMsg.BlockID = BlockID;
    GetMsg.DimCount = (int)Count.size();
    GetMsg.Count = Count.data();
    GetMsg.Start = Start.data();
    GetMsg.Dest = dest;
    CMwrite(m_conn, ev_state.GetRequestFormat, &GetMsg);
    return (Remote::GetHandle)(intptr_t)GetMsg.GetResponseCondition;
}

EVPathRemote::GetHandle EVPathRemote::Read(size_t Start, size_t Size, void *Dest)
{
    EVPathRemoteCommon::_ReadRequestMsg ReadMsg;
    memset(&ReadMsg, 0, sizeof(ReadMsg));
    ReadMsg.ReadResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    ReadMsg.FileHandle = m_ID;
    ReadMsg.Offset = Start;
    ReadMsg.Size = Size;
    ReadMsg.Dest = Dest;
    CMwrite(m_conn, ev_state.ReadRequestFormat, &ReadMsg);
    CMCondition_wait(ev_state.cm, ReadMsg.ReadResponseCondition);
    return (Remote::GetHandle)(intptr_t)ReadMsg.ReadResponseCondition;
}

bool EVPathRemote::WaitForGet(GetHandle handle)
{
    return CMCondition_wait(ev_state.cm, (int)(intptr_t)handle);
}
#else

void EVPathRemote::Open(const std::string hostname, const int32_t port, const std::string filename,
                        const Mode mode, bool RowMajorOrdering){};

void EVPathRemote::OpenSimpleFile(const std::string hostname, const int32_t port,
                                  const std::string filename){};

EVPathRemote::GetHandle EVPathRemote::Get(char *VarName, size_t Step, size_t BlockID, Dims &Count,
                                          Dims &Start, void *dest)
{
    return static_cast<GetHandle>(0);
};

bool EVPathRemote::WaitForGet(GetHandle handle) { return false; }

EVPathRemote::GetHandle EVPathRemote::Read(size_t Start, size_t Size, void *Dest)
{
    return static_cast<GetHandle>(0);
};
EVPathRemote::~EVPathRemote() {}
#endif

} // end namespace adios2
