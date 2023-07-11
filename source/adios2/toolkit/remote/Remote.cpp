/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */
#include "Remote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"

namespace adios2
{

Remote::Remote() {}

void OpenResponseHandler(CManager cm, CMConnection conn, void *vevent,
                         void *client_data, attr_list attrs)
{
    RemoteCommon::OpenResponseMsg open_response_msg =
        static_cast<RemoteCommon::OpenResponseMsg>(vevent);
    std::cout << "Got an open response for file "
              << open_response_msg->FileHandle << std::endl;

    void *obj = CMCondition_get_client_data(
        cm, open_response_msg->OpenResponseCondition);
    static_cast<Remote *>(obj)->m_ID = open_response_msg->FileHandle;
    CMCondition_signal(cm, open_response_msg->OpenResponseCondition);
    return;
};

void ReadResponseHandler(CManager cm, CMConnection conn, void *vevent,
                         void *client_data, attr_list attrs)
{
    RemoteCommon::ReadResponseMsg read_response_msg =
        static_cast<RemoteCommon::ReadResponseMsg>(vevent);
    std::cout << "Got an read response with " << read_response_msg->Size
              << " bytes of data" << std::endl;

    memcpy(read_response_msg->Dest, read_response_msg->ReadData,
           read_response_msg->Size);
    CMCondition_signal(cm, read_response_msg->ReadResponseCondition);
    return;
};

void Remote::Open(const std::string hostname, const int32_t port,
                  const std::string filename, const Mode mode)
{

    RemoteCommon::_OpenFileMsg open_msg;
    ev_state.cm = CManager_create();
    CMfork_comm_thread(ev_state.cm);
    attr_list contact_list = create_attr_list();
    atom_t CM_IP_PORT = -1;
    atom_t CM_IP_HOSTNAME = -1;
    CM_IP_HOSTNAME = attr_atom_from_string("IP_HOST");
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    add_attr(contact_list, CM_IP_HOSTNAME, Attr_String,
             (attr_value)hostname.c_str());
    add_attr(contact_list, CM_IP_PORT, Attr_Int4, (attr_value)port);
    RegisterFormats(ev_state);
    CMregister_handler(ev_state.OpenResponseFormat,
                       (CMHandlerFunc)OpenResponseHandler, &ev_state);
    CMregister_handler(ev_state.ReadResponseFormat,
                       (CMHandlerFunc)ReadResponseHandler, &ev_state);
    m_conn = CMinitiate_conn(ev_state.cm, contact_list);
    m_cm = ev_state.cm;
    if (!m_conn)
        return;

    memset(&open_msg, 0, sizeof(open_msg));
    open_msg.FileName = (char *)filename.c_str();
    open_msg.Mode = (RemoteCommon::REVPFileMode)mode;
    open_msg.OpenResponseCondition = CMCondition_get(m_cm, m_conn);
    CMCondition_set_client_data(m_cm, open_msg.OpenResponseCondition,
                                (void *)this);
    CMwrite(m_conn, ev_state.OpenFileFormat, &open_msg);
    CMCondition_wait(m_cm, open_msg.OpenResponseCondition);
    m_Active = true;
}

Remote::GetHandle Remote::Get(char *VarName, size_t Step, Dims &Count,
                              Dims &Start, void *dest)
{
    RemoteCommon::_GetRequestMsg GetMsg;
    memset(&GetMsg, 0, sizeof(GetMsg));
    GetMsg.GetResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    GetMsg.FileHandle = m_ID;
    GetMsg.VarName = VarName;
    GetMsg.Step = Step;
    GetMsg.DimCount = Count.size();
    GetMsg.Count = Count.data();
    GetMsg.Start = Start.data();
    GetMsg.Dest = dest;
    CMwrite(m_conn, ev_state.GetRequestFormat, &GetMsg);
    CMCondition_wait(m_cm, GetMsg.GetResponseCondition);
    return GetMsg.GetResponseCondition;
}

Remote::GetHandle Remote::Read(size_t Start, size_t Size, void *Dest)
{
    RemoteCommon::_ReadRequestMsg ReadMsg;
    memset(&ReadMsg, 0, sizeof(ReadMsg));
    ReadMsg.ReadResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    ReadMsg.FileHandle = m_ID;
    ReadMsg.Offset = Start;
    ReadMsg.Size = Size;
    ReadMsg.Dest = Dest;
    CMwrite(m_conn, ev_state.ReadRequestFormat, &ReadMsg);
    CMCondition_wait(m_cm, ReadMsg.ReadResponseCondition);
    return ReadMsg.ReadResponseCondition;
}

bool Remote::WaitForGet(GetHandle handle)
{
    return CMCondition_wait(ev_state.cm, (int)handle);
}

} // end namespace adios2
