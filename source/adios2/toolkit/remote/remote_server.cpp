#include <iostream>
#include <random>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"
#include <evpath.h>

#include "remote_common.h"

using namespace adios2::RemoteCommon;

using namespace adios2::core;
using namespace adios2;

int quiet = 1;
ADIOS adios("C++");

std::string lf_random_string()
{
    std::string str(
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    std::random_device rd;
    std::mt19937 generator(rd());

    std::shuffle(str.begin(), str.end(), generator);

    return str.substr(0, 8);
}

class AnonADIOSFile
{
public:
    IO *m_io = NULL;
    Engine *m_engine = NULL;
    int64_t m_ID;
    int64_t currentStep = -1;
    std::string m_IOname;
    AnonADIOSFile(std::string FileName)
    {
        m_IOname = lf_random_string();
        m_io = &adios.DeclareIO(m_IOname);
        m_engine = &m_io->Open(FileName, adios2::Mode::Read);
        memcpy(&m_ID, m_IOname.c_str(), sizeof(m_ID));
    }
    ~AnonADIOSFile()
    {
        std::cout << "Destroying file " << std::endl;
        m_engine->Close();
        adios.RemoveIO(m_IOname);
    }
};

std::unordered_map<uint64_t, AnonADIOSFile *> FileMap;

static void OpenHandler(CManager cm, CMConnection conn, void *vevent,
                        void *client_data, attr_list attrs)
{
    OpenFileMsg open_msg = static_cast<OpenFileMsg>(vevent);
    struct Remote_evpath_state *ev_state =
        static_cast<struct Remote_evpath_state *>(client_data);
    _OpenResponseMsg open_response_msg;
    std::cout << "Got an open request for file " << open_msg->FileName
              << std::endl;
    AnonADIOSFile *f = new AnonADIOSFile(open_msg->FileName);
    memset(&open_response_msg, 0, sizeof(open_response_msg));
    open_response_msg.FileHandle = f->m_ID;
    open_response_msg.OpenResponseCondition = open_msg->OpenResponseCondition;
    CMwrite(conn, ev_state->OpenResponseFormat, &open_response_msg);
    FileMap[f->m_ID] = f;
}

static void GetRequestHandler(CManager cm, CMConnection conn, void *vevent,
                              void *client_data, attr_list attrs)
{
    GetRequestMsg GetMsg = static_cast<GetRequestMsg>(vevent);
    AnonADIOSFile *f = FileMap[GetMsg->FileHandle];
    struct Remote_evpath_state *ev_state =
        static_cast<struct Remote_evpath_state *>(client_data);
    if (f->currentStep == -1)
    {
        f->m_engine->BeginStep();
        f->currentStep++;
    }
    while (f->m_engine->CurrentStep() < GetMsg->Step)
    {
        std::cout << "Advancing a step" << std::endl;
        f->m_engine->EndStep();
        f->m_engine->BeginStep();
        f->currentStep++;
    }

    std::string VarName = std::string(GetMsg->VarName);
    adios2::DataType TypeOfVar = f->m_io->InquireVariableType(VarName);
    if (GetMsg->Count)
    {
        // set count
    }
    if (GetMsg->Start)
    {
        // set start
    }

    if (TypeOfVar == adios2::DataType::None)
    {
    }
#define GET(T)                                                                 \
    else if (TypeOfVar == helper::GetDataType<T>())                            \
    {                                                                          \
        _ReadResponseMsg Response;                                             \
        memset(&Response, 0, sizeof(Response));                                \
        std::vector<T> RetData;                                                \
        auto var = f->m_io->InquireVariable<T>(VarName);                       \
        f->m_engine->Get(*var, RetData, Mode::Sync);                           \
        Response.Size = RetData.size() * sizeof(T);                            \
        Response.ReadData = (char *)RetData.data();                            \
        Response.ReadResponseCondition = GetMsg->GetResponseCondition;         \
        Response.Dest =                                                        \
            GetMsg->Dest; /* final data destination in client memory space */  \
        std::cout << "Returning " << Response.Size << " bytes for Get<"        \
                  << TypeOfVar << ">(" << VarName << ")" << std::endl;         \
        CMwrite(conn, ev_state->ReadResponseFormat, &Response);                \
    }
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(GET)
#undef GET
}

void REVPServerRegisterHandlers(struct Remote_evpath_state &ev_state)
{
    CMregister_handler(ev_state.OpenFileFormat, OpenHandler, &ev_state);
    CMregister_handler(ev_state.GetRequestFormat, GetRequestHandler, &ev_state);
}

static atom_t CM_IP_PORT = -1;
const int ServerPort = 26200;
int main(int argc, char **argv)
{
    CManager cm;
    struct Remote_evpath_state ev_state;

    (void)argc;
    (void)argv;
    cm = CManager_create();
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    attr_list listen_list = NULL;

    if (listen_list == NULL)
        listen_list = create_attr_list();
    add_attr(listen_list, CM_IP_PORT, Attr_Int4, (attr_value)ServerPort);
    CMlisten_specific(cm, listen_list);

    attr_list contact_list = CMget_contact_list(cm);
    if (contact_list)
    {
        char *string_list = attr_list_to_string(contact_list);
        std::cout << "Listening at port " << ServerPort << std::endl;
        free(string_list);
    }
    ev_state.cm = cm;

    while (argv[1] && (argv[1][0] == '-'))
    {
        if (argv[1][1] == 'v')
        {
            quiet--;
        }
        else if (argv[1][1] == 'q')
        {
            quiet++;
        }
        else if (argv[1][1] == '-')
        {
            argv++;
            argc--;
            break;
        }
        argv++;
        argc--;
    }

    RegisterFormats(ev_state);

    REVPServerRegisterHandlers(ev_state);

    std::cout << "doing Run Network" << std::endl;
    CMrun_network(cm);
    return 0;
}
