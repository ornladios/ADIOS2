#include <iostream>
#include <random>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/CoreTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosNetwork.h"
#include "adios2/operator/OperatorFactory.h"
#include <evpath.h>

#include <cstdio>  // remove
#include <cstring> // strerror
#include <errno.h> // errno
#include <fcntl.h> // open
#include <fstream>
#include <inttypes.h>
#include <iomanip>
#include <regex>
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#ifndef _MSC_VER
#include <mutex>
#include <thread>
#include <unistd.h> // write, close, ftruncate

static int fd_is_valid(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

#else
#include <io.h>
#define strdup _strdup
#define strlen _strlen
#define fileno _fileno
#define getpid _getpid
#define unlink _unlink
#define close _close
#define sleep(x) Sleep(x * 1000);
#define read _read
#define lseek _lseek
#define open _open
#endif

#include "remote_common.h"

using namespace adios2::EVPathRemoteCommon;

using namespace adios2::core;
using namespace adios2::helper;
using namespace adios2;

int verbose = 1;
ADIOS adios("C++");

size_t TotalSimpleBytesSent = 0;
size_t TotalGetBytesSent = 0;
size_t TotalSimpleReads = 0;
size_t TotalGets = 0;
size_t SimpleFilesOpened = 0;
size_t ADIOSFilesOpened = 0;
static int report_port_selection = 0;
int parent_pid;
uint64_t random_cookie = 0;
TimePoint startTime;

/* Threading for compressing responses of Gets */
size_t maxThreads = 8;
size_t nThreads = 0;
std::mutex mutex_nThreads;
std::mutex output_mutex;
char *log_filename = NULL;
std::ofstream fileOut;

static void log_output(const std::string out)
{
    std::lock_guard<std::mutex> lockGuard(output_mutex);
    static bool initialized = false;
    if (!initialized && log_filename)
    {
        // Opening the output file stream and associate it with
        // logfile
        fileOut.open(log_filename, std::ios::app);
        initialized = true;
    }
    if (log_filename)
    {
        fileOut << std::fixed << std::setprecision(3) << ((Seconds)(Now() - startTime)).count()
                << ": " << out << std::endl;
    }
    else
    {
        std::cout << std::fixed << std::setprecision(3) << ((Seconds)(Now() - startTime)).count()
                  << ": " << out << std::endl;
    }
}

void WaitForAvailableThread()
{
    // log_output("WaitForAvailableThread(): enter ");
    auto d = std::chrono::milliseconds(1000);
    while (true)
    {
        {
            std::lock_guard<std::mutex> lockGuard(mutex_nThreads);
            if (nThreads < maxThreads)
            {
                ++nThreads;
                // log_output("WaitForAvailableThread(): exit with threads = " +
                //        std::to_string(nThreads));
                break;
            }
        }
        // log_output("WaitForAvailableThread(): sleep = " + std::to_string(nThreads));
        std::this_thread::sleep_for(d);
    }
};

std::string readable_size(uint64_t size)
{
    constexpr const char FILE_SIZE_UNITS[8][3]{"B ", "KB", "MB", "GB", "TB", "PB", "EB", "ZB"};
    uint64_t s = size, r = 0;
    int idx = 0;
    while (s / 1024 > 0)
    {
        r = s % 1024;
        s = s / 1024;
        idx++;
    }
    int point = (int)(r / 100);
    std::ostringstream out;
    out << "" << s;
    if (point != 0)
        out << "." << point;
    out << " " << std::string(FILE_SIZE_UNITS[idx]);
    return out.str();
}

std::string lf_random_string()
{
    std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

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
    RemoteFileMode m_mode = EVPathRemoteCommon::RemoteFileMode::RemoteOpen;
    AnonADIOSFile(std::string FileName, EVPathRemoteCommon::RemoteFileMode mode,
                  bool RowMajorArrays)
    {
        Mode adios_read_mode = adios2::Mode::Read;
        m_FileName = FileName;
        m_IOname = lf_random_string();
        ArrayOrdering ArrayOrder =
            RowMajorArrays ? ArrayOrdering::RowMajor : ArrayOrdering::ColumnMajor;
        m_io = &adios.DeclareIO(m_IOname, ArrayOrder);
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
    size_t m_Size = (size_t)-1;
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
    ~AnonSimpleFile()
    {
        if (m_FileDescriptor != -1)
            close(m_FileDescriptor);
    }
};

std::unordered_map<uint64_t, AnonADIOSFile *> ADIOSFileMap;
std::unordered_map<uint64_t, std::shared_ptr<AnonSimpleFile>> SimpleFileMap;
std::unordered_multimap<void *, uint64_t> ConnToFileMap;
static auto last_service_time = std::chrono::steady_clock::now();

static void ConnCloseHandler(CManager cm, CMConnection conn, void *client_data)
{
    auto it = ConnToFileMap.equal_range(conn);
    for (auto it1 = it.first; it1 != it.second; it1++)
    {
        AnonADIOSFile *file = ADIOSFileMap[it1->second];
        if (file)
        {
            if (verbose >= 1)
                log_output("closing ADIOS file \"" + file->m_FileName + "\" total sent " +
                           readable_size(file->m_BytesSent) + " in " +
                           std::to_string(file->m_OperationCount) + " Get()s");
            ADIOSFileMap.erase(it1->second);
            delete file;
        }
        auto sfile = SimpleFileMap[it1->second];
        if (sfile)
        {
            if (verbose >= 1)
                log_output("closing simple file " + sfile->m_FileName + "\" total sent " +
                           readable_size(sfile->m_BytesSent) + " in " +
                           std::to_string(sfile->m_OperationCount) + " Read()s");
            SimpleFileMap.erase(it1->second);
        }
    }
    ConnToFileMap.erase(conn);
}

static void OpenHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                        attr_list attrs)
{
    OpenFileMsg open_msg = static_cast<OpenFileMsg>(vevent);
    AnonADIOSFile *f;
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    _OpenResponseMsg open_response_msg;
    memset(&open_response_msg, 0, sizeof(open_response_msg));
    open_response_msg.OpenResponseCondition = open_msg->OpenResponseCondition;
    std::string strMode = "Streaming";
    if (open_msg->Mode == RemoteOpenRandomAccess)
        strMode = "RandomAccess";
    try
    {
        f = new AnonADIOSFile(open_msg->FileName, open_msg->Mode, open_msg->RowMajorOrder);
        open_response_msg.FileHandle = f->m_ID;
    }
    catch (...)
    {
        open_response_msg.FileHandle = -1;
        CMwrite(conn, ev_state->OpenResponseFormat, &open_response_msg);
        return;
    }
    CMwrite(conn, ev_state->OpenResponseFormat, &open_response_msg);
    CMconn_register_close_handler(conn, ConnCloseHandler, NULL);
    ADIOSFileMap[f->m_ID] = f;
    ConnToFileMap.emplace(conn, f->m_ID);
    ADIOSFilesOpened++;
    last_service_time = std::chrono::steady_clock::now();
}

static void OpenSimpleHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                              attr_list attrs)
{
    _OpenSimpleResponseMsg open_response_msg;
    memset(&open_response_msg, 0, sizeof(open_response_msg));
    OpenSimpleFileMsg open_msg = static_cast<OpenSimpleFileMsg>(vevent);
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    try
    {
        auto f = std::make_shared<AnonSimpleFile>(open_msg->FileName);
        char *ContentsToFree = NULL;
        if (f->m_FileDescriptor == -1)
        {
            log_output("Simple Open failed! returning error!");
            throw std::runtime_error("open failed");
        }

        f->m_FileName = open_msg->FileName;
        open_response_msg.FileHandle = f->m_ID;
        open_response_msg.FileSize = f->m_Size;
        open_response_msg.OpenResponseCondition = open_msg->OpenResponseCondition;
        if (open_msg->ReadContents)
        {
            // This is a one-shot operation
            ContentsToFree = (char *)malloc(f->m_Size);
            size_t remaining = f->m_Size;
            char *pointer = ContentsToFree;
            while (remaining > 0)
            {
                ssize_t ret = read(f->m_FileDescriptor, pointer, (int)remaining);
                if (ret <= 0)
                {
                    log_output("Simple OpenRead failed! returning error!");
                    // instead free tmp and return;
                    free(ContentsToFree);
                    throw std::runtime_error("read failed");
                }
                else
                {
                    remaining -= ret;
                    pointer += ret;
                }
            }
            open_response_msg.FileContents = ContentsToFree;
            if (verbose >= 1)
                log_output("closing simple file after OpenRead" + f->m_FileName + "\" total sent " +
                           readable_size(f->m_Size));
        }
        else
        {
            // file is to remain open, keep records
            CMconn_register_close_handler(conn, ConnCloseHandler, NULL);
            SimpleFileMap[f->m_ID] = f;
            ConnToFileMap.emplace(conn, f->m_ID);
        }
        CMwrite(conn, ev_state->OpenSimpleResponseFormat, &open_response_msg);
        if (ContentsToFree)
            free(ContentsToFree);
        SimpleFilesOpened++;
        last_service_time = std::chrono::steady_clock::now();
        return;
    }
    catch (...)
    {
        open_response_msg.FileHandle = -1;
        open_response_msg.FileSize = (size_t)-1;
        open_response_msg.OpenResponseCondition = open_msg->OpenResponseCondition;
        CMwrite(conn, ev_state->OpenSimpleResponseFormat, &open_response_msg);
    }
}

template <class T>
void ReturnResponseThread(CMConnection conn, CMFormat ReadResponseFormat, AnonADIOSFile *f,
                          size_t readSize, T *RawData, void *Dest, int GetResponseCondition,
                          Accuracy acc, std::string name, adios2::DataType vartype,
                          size_t stepStart, size_t stepCount, adios2::Dims count,
                          adios2::Dims start, size_t blockid, bool boxselection)

{
    _ReadResponseMsg Response;
    memset(&Response, 0, sizeof(Response));
    Response.Size = readSize;
    Response.ReadResponseCondition = GetResponseCondition;
    Response.Dest = Dest; /* final data destination in client memory space */
    Response.OperatorType = Operator::OperatorType::COMPRESS_NULL;

    if (acc.error > 0.0)
    {
#if defined(ADIOS2_HAVE_MGARD) || defined(ADIOS2_HAVE_ZFP)
#if defined(ADIOS2_HAVE_MGARD)
        Params p = {{"accuracy", std::to_string(acc.error)},
                    {"s", std::to_string(acc.norm)},
                    {"mode", (acc.relative ? "REL" : "ABS")}};
        auto op = MakeOperator("mgard", p);
        log_output("    Compressing with mgard: " + name);
#elif defined(ADIOS2_HAVE_ZFP)
        Params p = {{"accuracy", std::to_string(acc.error)}};
        auto op = MakeOperator("zfp", p);
#endif
        // TODO: would be nicer:
        // op.SetAccuracy(Accuracy(GetMsg->error, GetMsg->norm, GetMsg->relative));
        T *CompressedData = (T *)malloc(Response.Size);
        log_output("    Allocated for compressed output: " + readable_size(Response.Size));
        adios2::Dims c;
        if (stepCount <= 1)
        {
            c = count;
        }
        else
        {
            c = helper::DimsWithStep(stepCount, count);
        }
        size_t result = op->Operate((char *)RawData, {}, c, vartype, (char *)CompressedData);
        log_output("    Compressed result size = " + readable_size(result));
        if (result == 0)
        {
            Response.ReadData = (char *)RawData;
            free(CompressedData);
        }
        else
        {
            Response.ReadData = (char *)CompressedData;
            Response.Size = result;
            Response.OperatorType = op->m_TypeEnum;
            free(RawData);
        }
#else
        Response.ReadData = (char *)RawData;
#endif
    }
    else
    {
        Response.ReadData = (char *)RawData;
    }

    if (verbose >= 2)
    {
        if (boxselection)
        {
            size_t stepEnd = stepStart + stepCount - 1;
            log_output("Returning " + readable_size(Response.Size) + " for Get<" +
                       adios2::ToString(vartype) + ">(" + name +
                       ") start = " + DimsToString(start) + " count = " + DimsToString(count) +
                       " steps = " + std::to_string(stepStart) + ".." + std::to_string(stepEnd));
        }
        else
        {
            log_output("Returning " + readable_size(Response.Size) + " for Get<" +
                       adios2::ToString(vartype) + ">(" + name +
                       ") block = " + std::to_string(blockid));
        }
    }
    CMwrite(conn, ReadResponseFormat, &Response);
    free(Response.ReadData);
    {
        std::lock_guard<std::mutex> lockGuard(mutex_nThreads);
        f->m_BytesSent += Response.Size;
        f->m_OperationCount++;
        TotalGetBytesSent += Response.Size;
        TotalGets++;
        --nThreads;
    }
}

template <class T>
void PrepareResponseForGet(CMConnection conn, struct Remote_evpath_state *ev_state,
                           GetRequestMsg GetMsg, std::string &VarName, adios2::DataType TypeOfVar,
                           AnonADIOSFile *f)
{
    // This part cannot be threaded as ADIOS InquireVariable/Get are not thread-safe
    Variable<T> *var = f->m_io->InquireVariable<T>(VarName);
    if (f->m_mode == RemoteOpenRandomAccess)
        var->SetStepSelection({GetMsg->Step, GetMsg->StepCount});
    if (GetMsg->BlockID != -1)
        var->SetBlockSelection(GetMsg->BlockID);
    if (GetMsg->Start)
    {
        Box<Dims> b;
        if (GetMsg->Count)
        {
            for (int i = 0; i < GetMsg->DimCount; i++)
            {
                b.first.push_back(GetMsg->Start[i]);
                b.second.push_back(GetMsg->Count[i]);
            }
        }
        var->SetSelection(b);
    }
    log_output("Reading var " + VarName + " with " + (GetMsg->Relative ? "relative" : "absolute") +
               " error " + std::to_string(GetMsg->Error) + " in norm " +
               std::to_string(GetMsg->Norm));
    size_t readSize = var->SelectionSize() * sizeof(T);
    T *RawData = (T *)malloc(readSize);
    try
    {
        f->m_engine->Get(*var, RawData, Mode::Sync);
    }
    catch (...)
    {
        log_output("Reading var " + VarName + " failed with exception, continuing");
        return;
    }
    WaitForAvailableThread(); /* blocking here until we can launch a thread */

    // Handle returning data in a separate thread so that we can serve another read operation in the
    // meantime. Compress data if possible and if requested.
    // Note: Can't pass Variable object to thread as other response will modify its content
    Accuracy acc = {GetMsg->Error, GetMsg->Norm, (bool)GetMsg->Relative};
    std::thread{ReturnResponseThread<T>,
                conn,
                ev_state->ReadResponseFormat,
                f,
                readSize,
                RawData,
                GetMsg->Dest,
                GetMsg->GetResponseCondition,
                acc,
                var->m_Name,
                var->m_Type,
                var->m_StepsStart,
                var->m_StepsCount,
                var->Count(), // function, not m_Count is correct for block selections
                var->m_Start,
                var->m_BlockID,
                (var->m_SelectionType == SelectionType::BoundingBox)}
        .detach();
    return;
}

static void GetRequestHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                              attr_list attrs)
{
    GetRequestMsg GetMsg = static_cast<GetRequestMsg>(vevent);
    AnonADIOSFile *f = ADIOSFileMap[GetMsg->FileHandle];
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    last_service_time = std::chrono::steady_clock::now();
    if (!f)
    {
        log_output("file not open, abort Get");
        return;
    }
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
                log_output("Advancing a step");
            f->m_engine->EndStep();
            f->m_engine->BeginStep();
            f->currentStep++;
        }
    }

    std::string VarName = std::string(GetMsg->VarName);
    adios2::DataType TypeOfVar = f->m_io->InquireVariableType(VarName);

    try
    {
        if (TypeOfVar == adios2::DataType::None)
        {
        }
#define GET(T)                                                                                     \
    else if (TypeOfVar == helper::GetDataType<T>())                                                \
    {                                                                                              \
        PrepareResponseForGet<T>(conn, ev_state, GetMsg, VarName, TypeOfVar, f);                   \
    }
        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(GET)
#undef GET
    }
    catch (const std::exception &exc)
    {
        if (verbose)
            log_output("Returning exception " + std::string(exc.what()) + " for Get<" +
                       ToString(TypeOfVar) + ">(" + VarName + ")");
    }
}

static void ReadRequestHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                               attr_list attrs)
{
    ReadRequestMsg ReadMsg = static_cast<ReadRequestMsg>(vevent);
    auto f = SimpleFileMap[ReadMsg->FileHandle];
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    last_service_time = std::chrono::steady_clock::now();
    if (f->m_CurrentOffset != ReadMsg->Offset)
    {
        lseek(f->m_FileDescriptor, (long)ReadMsg->Offset, SEEK_SET);
        f->m_CurrentOffset = ReadMsg->Offset;
    }
    char *tmp = (char *)malloc(ReadMsg->Size);
    size_t remaining = ReadMsg->Size;
    char *pointer = tmp;
    while (remaining > 0)
    {
        ssize_t ret = read(f->m_FileDescriptor, pointer, (int)remaining);
        if (ret <= 0)
        {
            // EOF or error,  should send a message back, but we haven't define error handling yet
            log_output("Read failed! BAD!");
            // instead free tmp and return;
            free(tmp);
            return;
        }
        else
        {
            remaining -= ret;
            pointer += ret;
        }
    }
    f->m_CurrentOffset += ReadMsg->Size;
    _ReadResponseMsg Response;
    memset(&Response, 0, sizeof(Response));
    Response.Size = ReadMsg->Size;
    Response.ReadData = (char *)tmp;
    Response.ReadResponseCondition = ReadMsg->ReadResponseCondition;
    Response.Dest = ReadMsg->Dest;
    Response.OperatorType = Operator::OperatorType::COMPRESS_NULL;
    if (verbose >= 2)
        log_output("Returning " + readable_size(Response.Size) + " for Read ");
    f->m_BytesSent += Response.Size;
    f->m_OperationCount++;
    TotalSimpleBytesSent += Response.Size;
    TotalSimpleReads++;
    CMwrite(conn, ev_state->ReadResponseFormat, &Response);
    free(tmp);
}

static void CloseFileHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                             attr_list attrs)
{
    CloseFileMsg CloseMsg = static_cast<CloseFileMsg>(vevent);
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    AnonADIOSFile *afile = ADIOSFileMap[CloseMsg->FileHandle];
    int Ret = 1; // failure
    if (afile)
    {
        if (verbose >= 1)
            log_output("closing ADIOS file \"" + afile->m_FileName + "\" total sent " +
                       readable_size(afile->m_BytesSent) + " in " +
                       std::to_string(afile->m_OperationCount) + " Get()s");
        ADIOSFileMap.erase(CloseMsg->FileHandle);
        delete afile;
        Ret = 0;
    }
    auto sfile = SimpleFileMap[CloseMsg->FileHandle];
    if (sfile)
    {
        if (verbose >= 1)
            log_output("closing simple file " + sfile->m_FileName + "\" total sent " +
                       readable_size(sfile->m_BytesSent) + " in " +
                       std::to_string(sfile->m_OperationCount) + " Read()s");
        SimpleFileMap.erase(CloseMsg->FileHandle);
        Ret = 0;
    }
    struct _CloseFileResponseMsg close_response_msg;
    memset(&close_response_msg, 0, sizeof(close_response_msg));
    close_response_msg.CloseResponseCondition = CloseMsg->CloseResponseCondition;
    close_response_msg.Status = Ret;
    CMwrite(conn, ev_state->CloseResponseFormat, &close_response_msg);
}

static void KillServerHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                              attr_list attrs)
{
    KillServerMsg kill_msg = static_cast<KillServerMsg>(vevent);
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    _KillResponseMsg kill_response_msg;
    memset(&kill_response_msg, 0, sizeof(kill_response_msg));
    kill_response_msg.KillResponseCondition = kill_msg->KillResponseCondition;
    std::stringstream Status;
    Status << "ADIOS files Opened: " << std::to_string(ADIOSFilesOpened) << " ("
           << std::to_string(TotalGets) << " gets for " << readable_size(TotalGetBytesSent)
           << ")  Simple files opened: " << std::to_string(SimpleFilesOpened) << " ("
           << TotalSimpleReads << " reads for " << readable_size(TotalSimpleBytesSent) << ")";
    kill_response_msg.Status = strdup(Status.str().c_str());
    CMwrite(conn, ev_state->KillResponseFormat, &kill_response_msg);
    free(kill_response_msg.Status);
    exit(0);
}

static void KillResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                                attr_list attrs)
{
    KillResponseMsg kill_response_msg = static_cast<KillResponseMsg>(vevent);
    std::cout << "Server final status: " << std::string(kill_response_msg->Status) << std::endl;
    CMCondition_signal(cm, kill_response_msg->KillResponseCondition);
}

static void StatusServerHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                                attr_list attrs)
{
    StatusServerMsg status_msg = static_cast<StatusServerMsg>(vevent);
    struct Remote_evpath_state *ev_state = static_cast<struct Remote_evpath_state *>(client_data);
    _StatusResponseMsg status_response_msg;
    char hostbuffer[256];

    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));
    memset(&status_response_msg, 0, sizeof(status_response_msg));
    status_response_msg.StatusResponseCondition = status_msg->StatusResponseCondition;
    status_response_msg.Hostname = &hostbuffer[0];
    std::stringstream Status;
    Status << "ADIOS files Opened: " << ADIOSFilesOpened << " (" << TotalGets << " gets for "
           << readable_size(TotalGetBytesSent) << ")  Simple files opened: " << SimpleFilesOpened
           << " (" << TotalSimpleReads << " reads for " << readable_size(TotalSimpleBytesSent)
           << ")";
    status_response_msg.Status = strdup(Status.str().c_str());
    CMwrite(conn, ev_state->StatusResponseFormat, &status_response_msg);
    free(status_response_msg.Status);
}

static void StatusResponseHandler(CManager cm, CMConnection conn, void *vevent, void *client_data,
                                  attr_list attrs)
{
    StatusResponseMsg status_response_msg = static_cast<StatusResponseMsg>(vevent);
    std::cout << "Server running on " << std::string(status_response_msg->Hostname)
              << " current status: " << std::string(status_response_msg->Status) << std::endl;
    CMCondition_signal(cm, status_response_msg->StatusResponseCondition);
}

void ServerRegisterHandlers(struct Remote_evpath_state &ev_state)
{
    CMregister_handler(ev_state.OpenFileFormat, OpenHandler, &ev_state);
    CMregister_handler(ev_state.OpenSimpleFileFormat, OpenSimpleHandler, &ev_state);
    CMregister_handler(ev_state.GetRequestFormat, GetRequestHandler, &ev_state);
    CMregister_handler(ev_state.ReadRequestFormat, ReadRequestHandler, &ev_state);
    CMregister_handler(ev_state.CloseFileFormat, CloseFileHandler, &ev_state);
    CMregister_handler(ev_state.KillServerFormat, KillServerHandler, &ev_state);
    CMregister_handler(ev_state.KillResponseFormat, KillResponseHandler, &ev_state);
    CMregister_handler(ev_state.StatusServerFormat, StatusServerHandler, &ev_state);
    CMregister_handler(ev_state.StatusResponseFormat, StatusResponseHandler, &ev_state);
}

static const char *hostname = "localhost";

void connect_and_kill(int ServerPort)
{
    CManager cm = CManager_create();
    _KillServerMsg kill_msg;
    struct Remote_evpath_state ev_state;
    attr_list contact_list = create_attr_list();
    atom_t CM_IP_PORT = -1;
    atom_t CM_IP_HOSTNAME = -1;
    CM_IP_HOSTNAME = attr_atom_from_string("IP_HOST");
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    add_attr(contact_list, CM_IP_HOSTNAME, Attr_String, (attr_value)strdup(hostname));
    add_attr(contact_list, CM_IP_PORT, Attr_Int4, (attr_value)ServerPort);
    CMConnection conn = CMinitiate_conn(cm, contact_list);
    if (!conn)
    {
        log_output("No server at destination \"" + std::string(hostname) + "\" port " +
                   std::to_string(ServerPort));
        free_attr_list(contact_list);
        CManager_close(cm);
        exit(0);
    }

    ev_state.cm = cm;

    RegisterFormats(ev_state);

    ServerRegisterHandlers(ev_state);

    memset(&kill_msg, 0, sizeof(kill_msg));
    kill_msg.KillResponseCondition = CMCondition_get(ev_state.cm, conn);
    CMwrite(conn, ev_state.KillServerFormat, &kill_msg);
    if (CMCondition_wait(ev_state.cm, kill_msg.KillResponseCondition) != 1)
    {
        log_output("Server existed, but no kill confirmation.  Maybe OK");
    }
    CMConnection_close(conn);
    CManager_close(ev_state.cm);
    exit(0);
}

void connect_and_get_status(int ServerPort)
{
    CManager cm = CManager_create();
    _StatusServerMsg status_msg;
    struct Remote_evpath_state ev_state;
    attr_list contact_list = create_attr_list();
    atom_t CM_IP_PORT = -1;
    atom_t CM_IP_HOSTNAME = -1;
    CM_IP_HOSTNAME = attr_atom_from_string("IP_HOST");
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    add_attr(contact_list, CM_IP_HOSTNAME, Attr_String, (attr_value)strdup(hostname));
    add_attr(contact_list, CM_IP_PORT, Attr_Int4, (attr_value)ServerPort);
    CMConnection conn = CMinitiate_conn(cm, contact_list);
    if (!conn)
    {
        log_output("No server at destination \"" + std::string(hostname) + "\" port " +
                   std::to_string(ServerPort));
        free_attr_list(contact_list);
        CManager_close(cm);
        exit(0);
    }

    ev_state.cm = cm;

    RegisterFormats(ev_state);

    ServerRegisterHandlers(ev_state);

    memset(&status_msg, 0, sizeof(status_msg));
    status_msg.StatusResponseCondition = CMCondition_get(ev_state.cm, conn);
    CMwrite(conn, ev_state.StatusServerFormat, &status_msg);
    if (CMCondition_wait(ev_state.cm, status_msg.StatusResponseCondition) != 1)
    {
        log_output("Server existed, but no status response...");
    }
    CMConnection_close(conn);
    CManager_close(ev_state.cm);
    exit(0);
}

static atom_t CM_IP_PORT = -1;

static bool server_timeout(void *CMvoid, int time_since_service)
{
    CManager cm = (CManager)CMvoid;
    if (verbose && (time_since_service > 90))
        log_output(std::to_string(time_since_service) + " seconds since last service.");
    if (time_since_service > 6000)
    {
        if (verbose)
            log_output("Timing out remote server");
        CManager_close(cm);
        return true;
    }
    return false;
}

#define IMAX_BITS(m) ((m) / ((m) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / ((m) % 255 + 12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

uint64_t rand64(void)
{
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH)
    {
        r += RAND_MAX_WIDTH;
        r ^= (unsigned)rand();
    }
    return r;
}

static void timer_start(void *param, unsigned int interval)
{

    std::thread([param, interval]() {
        while (true)
        {
            auto now = std::chrono::steady_clock::now();
            auto secs =
                std::chrono::duration_cast<std::chrono::seconds>(now - last_service_time).count();
            auto x = now + std::chrono::milliseconds(interval);
            if (server_timeout(param, (int)secs))
                return;
            std::this_thread::sleep_until(x);
        }
    }).detach();
}

const char usage[] = "Usage:  adios2_remote_server [-background] [-kill_server] [-no_timeout] "
                     "[-status] [-v] [-q] [-l logfile] [-t nthreads]\n";

int main(int argc, char **argv)
{
    CManager cm;
    struct Remote_evpath_state ev_state;
    int background = 0;
    int kill_server = 0;
    int status_server = 0;
    int no_timeout = 0; // default to timeout

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-background") == 0)
        {
            background++;
        }
        else if (strcmp(argv[i], "-kill_server") == 0)
        {
            kill_server++;
        }
        else if (strcmp(argv[i], "-report_port_selection") == 0)
        {
            report_port_selection++;
        }
        else if (strcmp(argv[i], "-status") == 0)
        {
            status_server++;
        }
        else if (strcmp(argv[i], "-no_timeout") == 0)
        {
            no_timeout++;
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            verbose++;
        }
        else if (strcmp(argv[i], "-q") == 0)
        {
            verbose--;
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            i++;
            if (argc <= i)
            {
                fprintf(stderr, "Flag -l requires an argument\n");
                fprintf(stderr, usage);
                exit(1);
            }
            log_filename = argv[i];
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            if (argc <= i)
            {
                fprintf(stderr, "Flag -d requires an argument\n");
                fprintf(stderr, usage);
                exit(1);
            }
            {
                std::ifstream file(argv[i]);
                if (file.fail())
                {
                    std::cerr << "Error opening file " << argv[i] << std::endl;
                    return 1;
                }
                std::string tp;
                while (std::getline(file, tp))
                    std::cout << tp << std::endl;
            }
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            i++;
            if (argc <= i)
            {
                fprintf(stderr, "Flag -t requires an argument\n");
                fprintf(stderr, usage);
                exit(1);
            }
            maxThreads = strtol(argv[i], nullptr, 10);
        }
        else
        {
            fprintf(stderr, "Unknown argument \"%s\"\n", argv[i]);
            fprintf(stderr, usage);
            exit(1);
        }
    }

    if (kill_server)
    {
        connect_and_kill(ServerPort);
        exit(0);
    }
    if (status_server)
    {
        connect_and_get_status(ServerPort);
        exit(0);
    }
    if (background)
    {
        if (verbose && !report_port_selection)
        {
            printf("Forking server to background\n");
        }
#ifdef _MSC_VER
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        char comm_line[8191];

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        char module[MAX_PATH];
        GetModuleFileName(NULL, &module[0], MAX_PATH);
        int i = 1;
        strcpy(comm_line, module);
        strcat(comm_line, " ");
        if (!CreateProcess(module, comm_line,
                           NULL,  // Process handle not inheritable
                           NULL,  // Thread handle not inheritable
                           FALSE, // Set handle inheritance to FALSE
                           0,     // No creation flags
                           NULL,  // Use parent's environment block
                           NULL,  // Use parent's starting directory
                           &si,   // Pointer to STARTUPINFO structure
                           &pi))
        {
            printf("CreateProcess failed (%d).\n", GetLastError());
            //	    printf("Args were argv[0] = %s\n", args[0]);
            //	    printf("Args were argv[1] = %s, argv[2] = %s\n", args[1], args[2]);
            exit(1);
        }
        exit(0);
#else
        parent_pid = getpid();
        if (fork() != 0)
        {
            /* I'm the parent, wait a sec to let the child start, then exit */
            //            sleep(1);
            if (report_port_selection)
            {
                char final_filename[256];
                snprintf(final_filename, sizeof(final_filename), "/tmp/port_file_%x", parent_pid);
                FILE *f = NULL;
                f = fopen(final_filename, "r");
                while (f == NULL)
                {
                    sleep(1); // wait until available
                    f = fopen(final_filename, "r");
                }
                char buffer[256];
                size_t ret = fread(buffer, 1, 256, f);
                // Note: printf("%s", buffer); does not seem to flush properly
                if (ret)
                    fprintf(stdout, "%s", buffer);
                fflush(stdout);
            }
            std::cout << "calling exit 0" << std::endl;
            ;
            close(0);
            close(1);
            close(2);
            exit(0);
        }

        //  Why close a bunch of FDs here?  Well, if we've linked with XRootD, stderr might have
        //  been dup()'d in library initialization.  We don't need those FDs and we have to
        //  close them to make sure we disassociate from the CTest parent (or else fixture
        //  startup hangs). It doesn't seem to work to close them before the fork, so we close
        //  them afterwards.
        auto file = freopen("/tmp/server_stdout", "a", stdout);
        if (!file)
            perror("freopen");
        file = freopen("/tmp/server_stderr", "a", stderr);
        if (!file)
            perror("freopen");
        for (int fd = 0; fd <= 16; fd++)
        {
            if (fd_is_valid(fd))
            {
                // OK, fd is valid, should we close it?
                if ((lseek(fd, 0, SEEK_CUR) == -1) && (errno == ESPIPE))
                {
                    // In the circumstances we care about (running under CTest), we want to
                    // close FDs that are pipes.  The condition above tests for that and we
                    // should get here only if it's a pipe.
                    close(fd);
                }
            }
        }
#endif
    }

    cm = CManager_create();
    if (!no_timeout)
        timer_start((void *)cm, 60 * 1000); // check timeout on 1 minute boundaries
    CM_IP_PORT = attr_atom_from_string("IP_PORT");
    attr_list listen_list = NULL;

    if (listen_list == NULL)
        listen_list = create_attr_list();
    if (!report_port_selection)
    {
        // listen on well-known port
        add_attr(listen_list, CM_IP_PORT, Attr_Int4, (attr_value)ServerPort);
        CMlisten_specific(cm, listen_list);
    }
    else
    {
        // randomize port
        CMlisten(cm);
        listen_list = CMget_contact_list(cm);
        int Port = -1;
        get_int_attr(listen_list, CM_IP_PORT, &Port);
        char filename[256];
        char final_filename[256];
        snprintf(filename, sizeof(filename), "/tmp/port_file_%x", getpid());
        snprintf(final_filename, sizeof(final_filename), "/tmp/port_file_%x", parent_pid);
        FILE *f = fopen(filename, "w");
        random_cookie = rand64();
        fprintf(f, "port:%d;msg:%s;cookie:%#018" PRIx64 "\n", Port, "no_error", random_cookie);
        fclose(f);
        rename(filename, final_filename);
    }

    startTime = Now();
    log_output("Hostname = " + helper::GetFQDN());
    attr_list contact_list = CMget_contact_list(cm);
    if (contact_list)
    {
        int Port = -1;
        get_int_attr(listen_list, CM_IP_PORT, &Port);
        log_output("Listening on Port " + std::to_string(Port));
    }
    ev_state.cm = cm;
    log_output("Max threads = " + std::to_string(maxThreads));

    RegisterFormats(ev_state);

    ServerRegisterHandlers(ev_state);

    CMrun_network(cm);
    return 0;
}
