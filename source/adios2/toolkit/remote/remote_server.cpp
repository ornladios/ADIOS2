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

#include <cstdio>  // remove
#include <cstring> // strerror
#include <errno.h> // errno
#include <fcntl.h> // open
#include <regex>
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close, ftruncate

#include "remote_common.h"

using namespace adios2::RemoteCommon;

using namespace adios2::core;
using namespace adios2;

int verbose = 1;
ADIOS adios("C++");

std::string readable_size(uint64_t size)
{
    constexpr const char FILE_SIZE_UNITS[8][3]{"B ", "KB", "MB", "GB",
                                               "TB", "PB", "EB", "ZB"};
    uint64_t s = size, r = 0;
    int idx = 0;
    while (s / 1024 > 0)
    {
        r = s % 1024;
        s = s / 1024;
        idx++;
    }
    int point = r / 100;
    std::ostringstream out;
    out << "" << s;
    if (point != 0)
        out << "." << point;
    out << " " << std::string(FILE_SIZE_UNITS[idx]);
    return out.str();
}

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
    std::string m_FileName;
    size_t m_BytesSent = 0;
    size_t m_OperationCount = 0;
    RemoteFileMode m_mode = RemoteCommon::RemoteFileMode::RemoteOpen;
    AnonADIOSFile(std::string FileName, RemoteCommon::RemoteFileMode mode)
    {
        Mode adios_read_mode = adios2::Mode::Read;
        m_FileName = FileName;
        m_IOname = lf_random_string();
        m_io = &adios.DeclareIO(m_IOname);
        m_mode = mode;
        if (m_mode == RemoteOpenRandomAccess)
            adios_read_mode = adios2::Mode::ReadRandomAccess;
        m_engine = &m_io->Open(FileName, adios_read_mode);
        memcpy(&m_ID, m_IOname.c_str(), sizeof(m_ID));
    }
    ~AnonADIOSFile()
    {
        m_engine->Close();
        adios.RemoveIO(m_IOname);
    }
};

class AnonSimpleFile
{
public:
    int64_t m_ID;
    int m_FileDescriptor;
    int m_Errno = 0;
    size_t m_Size = -1;
    size_t m_CurrentOffset = 0;
    std::string m_FileName;
    size_t m_BytesSent = 0;
    size_t m_OperationCount = 0;
    AnonSimpleFile(std::string FileName)
    {
        m_FileName = FileName;
        std::string tmpname = lf_random_string();
        struct stat fileStat;

        memcpy(&m_ID, tmpname.c_str(), sizeof(m_ID));
        errno = 0;
        m_FileDescriptor = open(FileName.c_str(), O_RDONLY);
        m_Errno = errno;
        if (fstat(m_FileDescriptor, &fileStat) == -1)
        {
            m_Errno = errno;
        }
        m_Size = static_cast<size_t>(fileStat.st_size);
    }
    ~AnonSimpleFile() { close(m_FileDescriptor); }
};

std::unordered_map<uint64_t, AnonADIOSFile *> ADIOSFileMap;
std::unordered_map<uint64_t, AnonSimpleFile *> SimpleFileMap;
std::unordered_multimap<void *, uint64_t> ConnToFileMap;

static void ConnCloseHandler(CManager cm, CMConnection conn, void *client_data)
{
    auto it = ConnToFileMap.equal_range(conn);
    for (auto it1 = it.first; it1 != it.second; it1++)
    {
        AnonADIOSFile *file = ADIOSFileMap[it1->second];
        if (file)
        {
            if (verbose >= 1)
                std::cout << "closing ADIOS file \"" << file->m_FileName
                          << "\" total sent "
                          << readable_size(file->m_BytesSent) << " in "
                          << file->m_OperationCount << " Get()s" << std::endl;
            ADIOSFileMap.erase(it1->second);
            delete file;
        }
        AnonSimpleFile *sfile = SimpleFileMap[it1->second];
        if (sfile)
        {
            if (verbose >= 1)
                std::cout << "closing simple file " << sfile->m_FileName
                          << "\" total sent "
                          << readable_size(sfile->m_BytesSent) << " in "
                          << sfile->m_OperationCount << " Read()s" << std::endl;
            SimpleFileMap.erase(it1->second);
            delete file;
        }
    }
    ConnToFileMap.erase(conn);
}

static void OpenHandler(CManager cm, CMConnection conn, void *vevent,
                        void *client_data, attr_list attrs)
{
    OpenFileMsg open_msg = static_cast<OpenFileMsg>(vevent);
    struct Remote_evpath_state *ev_state =
        static_cast<struct Remote_evpath_state *>(client_data);
    _OpenResponseMsg open_response_msg;
    std::cout << "Got an open request for file " << open_msg->FileName
              << std::endl;
    AnonADIOSFile *f = new AnonADIOSFile(open_msg->FileName, open_msg->Mode);
    memset(&open_response_msg, 0, sizeof(open_response_msg));
    open_response_msg.FileHandle = f->m_ID;
    open_response_msg.OpenResponseCondition = open_msg->OpenResponseCondition;
    CMwrite(conn, ev_state->OpenResponseFormat, &open_response_msg);
    CMconn_register_close_handler(conn, ConnCloseHandler, NULL);
    ADIOSFileMap[f->m_ID] = f;
    ConnToFileMap.emplace(conn, f->m_ID);
}

static void OpenSimpleHandler(CManager cm, CMConnection conn, void *vevent,
                              void *client_data, attr_list attrs)
{
    OpenSimpleFileMsg open_msg = static_cast<OpenSimpleFileMsg>(vevent);
    struct Remote_evpath_state *ev_state =
        static_cast<struct Remote_evpath_state *>(client_data);
    _OpenSimpleResponseMsg open_response_msg;
    std::cout << "Got an open simple request for file " << open_msg->FileName
              << std::endl;
    AnonSimpleFile *f = new AnonSimpleFile(open_msg->FileName);
    f->m_FileName = open_msg->FileName;
    memset(&open_response_msg, 0, sizeof(open_response_msg));
    open_response_msg.FileHandle = f->m_ID;
    open_response_msg.FileSize = f->m_Size;
    open_response_msg.OpenResponseCondition = open_msg->OpenResponseCondition;

    CMwrite(conn, ev_state->OpenSimpleResponseFormat, &open_response_msg);
    CMconn_register_close_handler(conn, ConnCloseHandler, NULL);
    SimpleFileMap[f->m_ID] = f;
    ConnToFileMap.emplace(conn, f->m_ID);
}

static void GetRequestHandler(CManager cm, CMConnection conn, void *vevent,
                              void *client_data, attr_list attrs)
{
    GetRequestMsg GetMsg = static_cast<GetRequestMsg>(vevent);
    AnonADIOSFile *f = ADIOSFileMap[GetMsg->FileHandle];
    struct Remote_evpath_state *ev_state =
        static_cast<struct Remote_evpath_state *>(client_data);
    if (f->m_mode == RemoteOpen)
    {
        if (f->currentStep == -1)
        {
            f->m_engine->BeginStep();
            f->currentStep++;
        }
        while (f->m_engine->CurrentStep() < GetMsg->Step)
        {
            if (verbose >= 2)
                std::cout << "Advancing a step" << std::endl;
            f->m_engine->EndStep();
            f->m_engine->BeginStep();
            f->currentStep++;
        }
    }

    std::string VarName = std::string(GetMsg->VarName);
    adios2::DataType TypeOfVar = f->m_io->InquireVariableType(VarName);
    Box<Dims> b;
    if (GetMsg->Count)
    {
        for (int i = 0; i < GetMsg->DimCount; i++)
        {
            b.first.push_back(GetMsg->Start[i]);
            b.second.push_back(GetMsg->Count[i]);
        }
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
        if (f->m_mode == RemoteOpenRandomAccess)                               \
            var->SetStepSelection({GetMsg->Step, 1});                          \
        if (GetMsg->BlockID != -1)                                             \
            var->SetBlockSelection(GetMsg->BlockID);                           \
	if (GetMsg->Start) 						       \
            var->SetSelection(b);                                              \
        f->m_engine->Get(*var, RetData, Mode::Sync);                           \
        Response.Size = RetData.size() * sizeof(T);                            \
        Response.ReadData = (char *)RetData.data();                            \
        Response.ReadResponseCondition = GetMsg->GetResponseCondition;         \
        Response.Dest =                                                        \
            GetMsg->Dest; /* final data destination in client memory space */  \
        if (verbose >= 2)                                                      \
            std::cout << "Returning " << Response.Size << " "                  \
                      << readable_size(Response.Size) << " for Get<"           \
                      << TypeOfVar << ">(" << VarName << ")" << b              \
                      << std::endl;                                            \
        f->m_BytesSent += Response.Size;                                       \
        f->m_OperationCount++;                                                 \
        CMwrite(conn, ev_state->ReadResponseFormat, &Response);                \
    }
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(GET)
#undef GET
}

static void ReadRequestHandler(CManager cm, CMConnection conn, void *vevent,
                               void *client_data, attr_list attrs)
{
    ReadRequestMsg ReadMsg = static_cast<ReadRequestMsg>(vevent);
    AnonSimpleFile *f = SimpleFileMap[ReadMsg->FileHandle];
    struct Remote_evpath_state *ev_state =
        static_cast<struct Remote_evpath_state *>(client_data);
    if (f->m_CurrentOffset != ReadMsg->Offset)
    {
        lseek(f->m_FileDescriptor, ReadMsg->Offset, SEEK_SET);
        f->m_CurrentOffset = ReadMsg->Offset;
    }
    char *tmp = (char *)malloc(ReadMsg->Size);
    read(f->m_FileDescriptor, tmp, ReadMsg->Size);
    f->m_CurrentOffset += ReadMsg->Size;
    _ReadResponseMsg Response;
    memset(&Response, 0, sizeof(Response));
    Response.Size = ReadMsg->Size;
    Response.ReadData = (char *)tmp;
    Response.ReadResponseCondition = ReadMsg->ReadResponseCondition;
    Response.Dest = ReadMsg->Dest;
    if (verbose >= 2)
        std::cout << "Returning " << readable_size(Response.Size)
                  << " for Read " << std::endl;
    f->m_BytesSent += Response.Size;
    f->m_OperationCount++;
    CMwrite(conn, ev_state->ReadResponseFormat, &Response);
    free(tmp);
}

void REVPServerRegisterHandlers(struct Remote_evpath_state &ev_state)
{
    CMregister_handler(ev_state.OpenFileFormat, OpenHandler, &ev_state);
    CMregister_handler(ev_state.OpenSimpleFileFormat, OpenSimpleHandler,
                       &ev_state);
    CMregister_handler(ev_state.GetRequestFormat, GetRequestHandler, &ev_state);
    CMregister_handler(ev_state.ReadRequestFormat, ReadRequestHandler,
                       &ev_state);
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
        size_t i = 1;
        while (argv[1][i] != 0)
        {
            if (argv[1][i] == 'v')
            {
                verbose++;
            }
            else if (argv[1][i] == 'q')
            {
                verbose--;
            }
            i++;
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
