/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 */
#include <chrono>
#include <thread>

#include "EVPathRemote.h"
#include "Remote.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Operator.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosString.h"
#include "adios2/helper/adiosSystem.h"
#include "adios2/operator/OperatorFactory.h"

#ifdef _MSC_VER
#define strdup(x) _strdup(x)
#endif

namespace adios2
{

EVPathRemote::EVPathRemote(const adios2::HostOptions &hostOptions) : Remote(hostOptions) {}

#ifdef ADIOS2_HAVE_SST
EVPathRemote::~EVPathRemote()
{
    if (m_conn)
        CMConnection_close(m_conn);
    m_conn = NULL;
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
    std::vector<char> *Tmp = static_cast<EVPathRemote *>(obj)->m_TmpContentVector;
    if (Tmp && open_response_msg->FileContents)
    {
        Tmp->resize(open_response_msg->FileSize);
        memcpy(Tmp->data(), open_response_msg->FileContents, open_response_msg->FileSize);
    }

    CMCondition_signal(cm, open_response_msg->OpenResponseCondition);
    return;
};

void CloseResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                          attr_list attrs)
{
    EVPathRemoteCommon::CloseFileResponseMsg close_response_msg =
        static_cast<EVPathRemoteCommon::CloseFileResponseMsg>(vevent);

    CMCondition_signal(cm, close_response_msg->CloseResponseCondition);
    return;
};

void ReadResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                         attr_list attrs)
{
    EVPathRemoteCommon::ReadResponseMsg read_response_msg =
        static_cast<EVPathRemoteCommon::ReadResponseMsg>(vevent);

    void *obj = CMCondition_get_client_data(cm, read_response_msg->ReadResponseCondition);
    CMtake_buffer(cm, read_response_msg);
    {
        const std::lock_guard<std::mutex> lock(static_cast<EVPathRemote *>(obj)->m_ResponsesMutex);
        static_cast<EVPathRemote *>(obj)->m_Responses.emplace(
            read_response_msg->ReadResponseCondition, read_response_msg);
    }
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
        CMregister_handler(ev_state.CloseResponseFormat, (CMHandlerFunc)CloseResponseHandler,
                           &ev_state);
    });
}

void EVPathRemote::Open(const std::string hostname, const int32_t port, const std::string filename,
                        const Mode mode, bool RowMajorOrdering)
{

    EVPathRemoteCommon::_OpenFileMsg open_msg;
    if (!m_Active)
    {
        InitCMData();
        attr_list contact_list = create_attr_list();
        atom_t CM_IP_PORT = -1;
        atom_t CM_IP_HOSTNAME = -1;
        CM_IP_HOSTNAME = attr_atom_from_string("IP_HOST");
        CM_IP_PORT = attr_atom_from_string("IP_PORT");
        add_attr(contact_list, CM_IP_HOSTNAME, Attr_String, (attr_value)strdup(hostname.c_str()));
        add_attr(contact_list, CM_IP_PORT, Attr_Int4, (attr_value)port);
        m_conn = CMinitiate_conn(ev_state.cm, contact_list);
        if ((m_conn == NULL) && (getenv("DoRemote") || getenv("DoFileRemote")))
        {
            // if we didn't find a server, but we're in testing, wait briefly and once more
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            m_conn = CMinitiate_conn(ev_state.cm, contact_list);
        }
        free_attr_list(contact_list);
        if (!m_conn)
        {
            helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenADIOSFile",
                                              "Failed to connect to remote server");
        }
    }

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
    if (CMCondition_wait(ev_state.cm, open_msg.OpenResponseCondition) != 1)
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenADIOSFile",
                                          "Failed to receive open acknowledgement, server failed?");
    }
    if (m_ID == -1)
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenADIOSFile",
                                          "Failed to Open ADIOS file \"" + filename + "\"");
    }
    m_Active = true;
}

void EVPathRemote::OpenSimpleFile(const std::string hostname, const int32_t port,
                                  const std::string filename)
{

    EVPathRemoteCommon::_OpenSimpleFileMsg open_msg;
    if (!m_Active)
    {
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
        {
            helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenSimpleFile",
                                              "Failed to connect to remote server");
        }
    }

    memset(&open_msg, 0, sizeof(open_msg));
    open_msg.FileName = (char *)filename.c_str();
    open_msg.OpenResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    open_msg.ReadContents = 0;
    CMCondition_set_client_data(ev_state.cm, open_msg.OpenResponseCondition, (void *)this);
    CMwrite(m_conn, ev_state.OpenSimpleFileFormat, &open_msg);
    if (CMCondition_wait(ev_state.cm, open_msg.OpenResponseCondition) != 1)
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenSimpleFile",
                                          "Failed to receive open acknowledgement, server failed?");
    }
    if ((m_ID == -1) && (m_Size == (size_t)-1))
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenSimpleFile",
                                          "Failed to open simple file \"" + filename + "\"");
    }
    else
    {
        m_Active = true;
    }
}

void EVPathRemote::OpenReadSimpleFile(const std::string hostname, const int32_t port,
                                      const std::string filename, std::vector<char> &contents)
{

    EVPathRemoteCommon::_OpenSimpleFileMsg open_msg;
    if (!m_Active)
    {
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
        {
            helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenReadSimpleFile",
                                              "Failed to connect to remote server");
        }
    }
    memset(&open_msg, 0, sizeof(open_msg));
    open_msg.FileName = (char *)filename.c_str();
    open_msg.OpenResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    open_msg.ReadContents = 1;
    CMCondition_set_client_data(ev_state.cm, open_msg.OpenResponseCondition, (void *)this);
    m_TmpContentVector = &contents; // this will be accessed in the handler
    CMwrite(m_conn, ev_state.OpenSimpleFileFormat, &open_msg);
    if (CMCondition_wait(ev_state.cm, open_msg.OpenResponseCondition) != 1)
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "OpenReadSimpleFile",
                                          "Failed to receive open acknowledgement, server failed?");
    }
    // file does not remain open after OpenReadSimpleFile
    m_TmpContentVector = nullptr;
    m_Active = false;
}

void EVPathRemote::Close()
{

    EVPathRemoteCommon::_CloseFileMsg CloseMsg;
    memset(&CloseMsg, 0, sizeof(CloseMsg));
    CloseMsg.CloseResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    CloseMsg.FileHandle = m_ID;
    CMCondition_set_client_data(ev_state.cm, CloseMsg.CloseResponseCondition, (void *)this);
    CMwrite(m_conn, ev_state.CloseFileFormat, &CloseMsg);
    if (CMCondition_wait(ev_state.cm, CloseMsg.CloseResponseCondition) != 1)
    {
        helper::Throw<std::runtime_error>(
            "Remote", "EVPathRemote", "CloseFile",
            "Failed to receive close acknowledgement, server failed?");
    }
    m_Active = false;
    m_ID = 0;
}

EVPathRemote::GetHandle EVPathRemote::Get(const char *VarName, size_t Step, size_t StepCount,
                                          size_t BlockID, Dims &Count, Dims &Start,
                                          Accuracy &accuracy, void *dest)
{
    EVPathRemoteCommon::_GetRequestMsg GetMsg;
    if (!m_Active)
        helper::Throw<std::invalid_argument>("Remote", "EVPathRemoteFile", "FileNotOpen",
                                             "Attempted a Get on an unopened file\n");

    memset(&GetMsg, 0, sizeof(GetMsg));
    GetMsg.GetResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    GetMsg.FileHandle = m_ID;
    GetMsg.VarName = VarName;
    GetMsg.Step = Step;
    GetMsg.StepCount = StepCount;
    GetMsg.BlockID = BlockID;
    GetMsg.DimCount = (int)Count.size();
    GetMsg.Count = Count.data();
    GetMsg.Start = Start.data();
    GetMsg.Error = accuracy.error;
    GetMsg.Norm = accuracy.norm;
    GetMsg.Relative = accuracy.relative;
    GetMsg.Dest = dest;
    CMCondition_set_client_data(ev_state.cm, GetMsg.GetResponseCondition, (void *)this);
    CMwrite(m_conn, ev_state.GetRequestFormat, &GetMsg);
    return (Remote::GetHandle)(intptr_t)GetMsg.GetResponseCondition;
}

void EVPathRemote::ProcessReadResponse(GetHandle handle)
{
    std::map<int, adios2::EVPathRemoteCommon::ReadResponseMsg>::iterator it;
    {
        const std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        it = m_Responses.find((int)(intptr_t)handle);
    }
    if (it == m_Responses.end())
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "WaitForGet",
                                          "Handle " + std::to_string((int)(intptr_t)handle) +
                                              " not found in list of responses");
    }

    EVPathRemoteCommon::ReadResponseMsg read_response_msg = it->second;
    switch (read_response_msg->OperatorType)
    {

    case adios2::core::Operator::OperatorType::COMPRESS_MGARD: {
        auto op = adios2::core::MakeOperator("mgard", {});
        op->InverseOperate(read_response_msg->ReadData, read_response_msg->Size,
                           (char *)read_response_msg->Dest);
        break;
    }

    case adios2::core::Operator::OperatorType::COMPRESS_ZFP: {
        auto op = adios2::core::MakeOperator("zfp", {});
        op->InverseOperate(read_response_msg->ReadData, read_response_msg->Size,
                           (char *)read_response_msg->Dest);
        break;
    }

    case adios2::core::Operator::OperatorType::COMPRESS_NULL:
        memcpy(read_response_msg->Dest, read_response_msg->ReadData, read_response_msg->Size);
        break;
    default:
        helper::Throw<std::invalid_argument>("Remote", "EVPathRemote", "ReadResponseHandler",
                                             "Invalid operator type " +
                                                 std::to_string(read_response_msg->OperatorType) +
                                                 " received in response");
    }
    CMreturn_buffer(ev_state.cm, read_response_msg);
}

bool EVPathRemote::WaitForGet(GetHandle handle)
{
    int result = CMCondition_wait(ev_state.cm, (int)(intptr_t)handle);
    if (result != 1)
    {
        helper::Throw<std::runtime_error>("Remote", "EVPathRemote", "Wait for Read/Get",
                                          "No Remote Read acknowledgement, server failed?");
    }
    ProcessReadResponse(handle);
    return result;
}

EVPathRemote::GetHandle EVPathRemote::Read(size_t Start, size_t Size, void *Dest)
{
    EVPathRemoteCommon::_ReadRequestMsg ReadMsg;
    if (!m_Active)
        helper::Throw<std::invalid_argument>("Remote", "EVPathRemoteFile", "FileNotOpen",
                                             "Attempted a Read on an unopened file\n");
    memset(&ReadMsg, 0, sizeof(ReadMsg));
    ReadMsg.ReadResponseCondition = CMCondition_get(ev_state.cm, m_conn);
    ReadMsg.FileHandle = m_ID;
    ReadMsg.Offset = Start;
    ReadMsg.Size = Size;
    ReadMsg.Dest = Dest;
    CMCondition_set_client_data(ev_state.cm, ReadMsg.ReadResponseCondition, (void *)this);
    CMwrite(m_conn, ev_state.ReadRequestFormat, &ReadMsg);
    WaitForGet((Remote::GetHandle)(intptr_t)ReadMsg.ReadResponseCondition);
    return (Remote::GetHandle)(intptr_t)ReadMsg.ReadResponseCondition;
}

std::map<std::string, std::pair<std::shared_ptr<EVPathRemote>, int>>
    CManagerSingleton::m_EVPathRemotes;

std::pair<std::shared_ptr<EVPathRemote>, int>
CManagerSingleton::MakeEVPathConnection(const std::string &hostName)
{
    auto it = m_EVPathRemotes.find(hostName);
    if (it != m_EVPathRemotes.end())
    {
        if (it->second.first && *(it->second.first))
        {
            return it->second;
        }
    }
    auto m_Remote =
        std::shared_ptr<EVPathRemote>(new EVPathRemote(core::ADIOS::StaticGetHostOptions()));
    try
    {
        int localPort = m_Remote->LaunchRemoteServerViaConnectionManager(hostName);
        std::pair<std::shared_ptr<EVPathRemote>, int> pair(m_Remote, localPort);
        m_EVPathRemotes.emplace(hostName, pair);
        return pair;
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << e.what() << '\n';
    }
    return std::pair<std::shared_ptr<EVPathRemote>, int>(nullptr, -1);
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
