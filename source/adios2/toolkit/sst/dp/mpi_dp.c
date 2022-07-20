#include "dp_interface.h"
#include "sst_data.h"
#include <adios2-perfstubs-interface.h>

#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MPI_DP_CONTACT_STRING_LEN 64

static pthread_rwlock_t LockRS = PTHREAD_RWLOCK_INITIALIZER;
static pthread_rwlock_t LockTS = PTHREAD_RWLOCK_INITIALIZER;
static pthread_once_t OnceMpiInitializer = PTHREAD_ONCE_INIT;

/*****Stream Basic Structures ***********************************************/

/* Base Stream class, used implicitly */
typedef struct _MpiStream
{
    void *CP_Stream;
    int Rank;
    int PID;
} MpiStream;

/* Link Stream class, used implicitly */
typedef struct _MpiStreamLink
{
    int CohortSize;
    CP_PeerCohort PeerCohort;
    SstStats Stats;
} MpiStreamLink;

/**
 * Readers Stream.
 *
 * It contains the needed data to communicate with a single Writer.
 */
typedef struct _MpiStreamRD
{
    MpiStream Stream;
    MpiStreamLink Link;

    CMFormat ReadRequestFormat;
    struct _MpiWriterContactInfo *WriterContactInfo;
} * MpiStreamRD;

/**
 * Writers Stream.
 *
 * It does not directly contains data related to each of the connected Readers.
 * Instead it contains a collection of MpiStreamWSR that represent the Stream
 * used for interacting with each (one/multiple) of the Readers.
 */
typedef struct _MpiStreamWS
{
    MpiStream Stream;

    struct _TimeStepEntry *TimeSteps;
    CMFormat ReadReplyFormat;
    int ReaderCount;
    struct _MpiStreamWSR **Readers;
    pthread_mutex_t MutexReaders;
} * MpiStreamWS;

/**
 * WritersPerReader streams.
 *
 * It is used in the Writer side to represent the Stream used for communicated
 * with a single Reader.
 */
typedef struct _MpiStreamWSR
{
    MpiStreamLink Link;

    struct _MpiStreamWS *StreamWS;
    struct _MpiReaderContactInfo *ReaderContactInfo;
    char MpiPortName[MPI_MAX_PORT_NAME];
} * MpiStreamWSR;

typedef struct _MpiPerTimeStepInfo
{
    char *CheckString;
} * MpiPerTimeStepInfo;

typedef struct _TimeStepEntry
{
    long TimeStep;
    struct _SstData *Data;
    struct _MpiPerTimeStepInfo *DP_TimeStepInfo;
    struct _TimeStepEntry *Next;
} * TimeStepList;

typedef struct _MpiReaderContactInfo
{
    char *ContactString;
    void *StreamRS;
    MPI_Comm MpiComm;
} * MpiReaderContactInfo;

typedef struct _MpiWriterContactInfo
{
    char *ContactString;
    void *StreamWS;
    MPI_Comm MpiComm;
    int PID;
} * MpiWriterContactInfo;

/*****Message Data Structures ***********************************************/

/**
 * Represents where the connection of two streams takes places:
 * Remotly|Locally.
 */
enum MPI_DP_COMM_TYPE
{
    MPI_DP_REMOTE = 0,
    MPI_DP_LOCAL = 1,
};

typedef struct _MpiReadRequestMsg
{
    long TimeStep;
    size_t Offset;
    size_t Length;
    void *StreamWS;
    void *StreamRS;
    int RequestingRank;
    int NotifyCondition;
    enum MPI_DP_COMM_TYPE CommType;
} * MpiReadRequestMsg;

typedef struct _MpiReadReplyMsg
{
    long TimeStep;
    size_t DataLength;
    void *StreamRS;
    int NotifyCondition;
    char *MpiPortName;
    char *Data;
} * MpiReadReplyMsg;

static FMField MpiReadRequestList[] = {
    {"TimeStep", "integer", sizeof(long),
     FMOffset(MpiReadRequestMsg, TimeStep)},
    {"Offset", "integer", sizeof(size_t), FMOffset(MpiReadRequestMsg, Offset)},
    {"Length", "integer", sizeof(size_t), FMOffset(MpiReadRequestMsg, Length)},
    {"StreamWS", "integer", sizeof(void *),
     FMOffset(MpiReadRequestMsg, StreamWS)},
    {"StreamRS", "integer", sizeof(void *),
     FMOffset(MpiReadRequestMsg, StreamRS)},
    {"RequestingRank", "integer", sizeof(int),
     FMOffset(MpiReadRequestMsg, RequestingRank)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(MpiReadRequestMsg, NotifyCondition)},
    {"CommType", "integer", sizeof(int), FMOffset(MpiReadRequestMsg, CommType)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MpiReadRequestStructs[] = {
    {"MpiReadRequest", MpiReadRequestList, sizeof(struct _MpiReadRequestMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static FMField MpiReadReplyList[] = {
    {"TimeStep", "integer", sizeof(long), FMOffset(MpiReadReplyMsg, TimeStep)},
    {"StreamRS", "integer", sizeof(void *),
     FMOffset(MpiReadReplyMsg, StreamRS)},
    {"DataLength", "integer", sizeof(size_t),
     FMOffset(MpiReadReplyMsg, DataLength)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(MpiReadReplyMsg, NotifyCondition)},
    {"MpiPortName", "string", sizeof(char *),
     FMOffset(MpiReadReplyMsg, MpiPortName)},
    {"Data", "char[DataLength]", sizeof(char), FMOffset(MpiReadReplyMsg, Data)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MpiReadReplyStructs[] = {
    {"MpiReadReply", MpiReadReplyList, sizeof(struct _MpiReadReplyMsg), NULL},
    {NULL, NULL, 0, NULL}};

static FMField MpiReaderContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(MpiReaderContactInfo, ContactString)},
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(MpiReaderContactInfo, StreamRS)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MpiReaderContactStructs[] = {
    {"MpiReaderContactInfo", MpiReaderContactList,
     sizeof(struct _MpiReaderContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField MpiWriterContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(MpiWriterContactInfo, ContactString)},
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(MpiWriterContactInfo, StreamWS)},
    {"MpiComm", "integer", sizeof(int),
     FMOffset(MpiWriterContactInfo, MpiComm)},
    {"PID", "integer", sizeof(int), FMOffset(MpiWriterContactInfo, PID)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MpiWriterContactStructs[] = {
    {"MpiWriterContactInfo", MpiWriterContactList,
     sizeof(struct _MpiWriterContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

/*****Internal functions*****************************************************/

static void MpiReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                void *client_Data, attr_list attrs);

/**
 * Initialize MPI in the mode that it is required for MPI_DP to work.
 *
 * It can be called multiple times.
 */
static void MpiInitialize()
{
    int IsInitialized = 0;
    int provided;

    MPI_Initialized(&IsInitialized);
    if (!IsInitialized)
    {
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
    }
    else
    {
        MPI_Query_thread(&provided);
    }

    if (provided != MPI_THREAD_MULTIPLE)
    {
        fprintf(stderr,
                "MPI init without MPI_THREAD_MULTIPLE (Externally "
                "initialized:%s)\n",
                IsInitialized ? "true" : "false");
    }
}

/*****Public accessible functions********************************************/

/**
 * InitReader.
 *
 * Called by the control plane collectively during the early stages of Open on
 * the reader side.  It should do whatever is necessary to initialize a new
 * reader-side data plane.  A pointer to per-reader-rank contact information
 * should be placed in *ReaderContactInfoPtr.  The structure of that
 * information should be described by DPInterface.ReaderContactFormats.  (This
 * is an FFS format description.  See
 * https://www.cc.gatech.edu/systems/projects/FFS/.)
 */
static DP_RS_Stream MpiInitReader(CP_Services Svcs, void *CP_Stream,
                                  void **ReaderContactInfoPtr,
                                  struct _SstParams *Params,
                                  attr_list WriterContact, SstStats Stats)
{
    pthread_once(&OnceMpiInitializer, MpiInitialize);

    MpiStreamRD Stream = calloc(sizeof(struct _MpiStreamRD), 1);
    MpiReaderContactInfo Contact =
        calloc(sizeof(struct _MpiReaderContactInfo), 1);
    CManager cm = Svcs->getCManager(CP_Stream);
    char *MpiContactString = calloc(sizeof(char), MPI_DP_CONTACT_STRING_LEN);
    SMPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    Stream->Stream.CP_Stream = CP_Stream;
    Stream->Stream.PID = getpid();
    Stream->Link.Stats = Stats;

    SMPI_Comm_rank(comm, &Stream->Stream.Rank);

    snprintf(MpiContactString, MPI_DP_CONTACT_STRING_LEN, "Reader Rank %d",
             Stream->Stream.Rank);

    /*
     * add a handler for read reply messages
     */
    Stream->ReadRequestFormat = CMregister_format(cm, MpiReadRequestStructs);
    F = CMregister_format(cm, MpiReadReplyStructs);
    CMregister_handler(F, MpiReadReplyHandler, Svcs);

    Contact->ContactString = MpiContactString;
    Contact->StreamRS = Stream;

    *ReaderContactInfoPtr = Contact;

    Svcs->verbose(Stream->Stream.CP_Stream, DPTraceVerbose,
                  "MPI dataplane reader initialized, reader rank %d",
                  Stream->Stream.Rank);

    return Stream;
}

static char *FetchTimeStep(TimeStepList timesteps, long timestep, long offset,
                           long length)
{
    TimeStepList ts = timesteps;

    pthread_rwlock_rdlock(&LockTS);

    // Find the requested timestep
    while (ts != NULL && ts->TimeStep != timestep)
    {
        ts = ts->Next;
    }

    if (ts == NULL)
    {
        fprintf(stderr, "Failed to read TimeStep %ld, not found\n", timestep);
        return NULL;
    }

    char *outboundBuffer = malloc(sizeof(char) * length);
    memcpy(outboundBuffer, ts->Data->block + offset, length);

    pthread_rwlock_unlock(&LockTS);

    return outboundBuffer;
}

static void MpiReadRequestHandler(CManager cm, CMConnection conn, void *msg_v,
                                  void *client_Data, attr_list attrs)
{
    MpiReadRequestMsg ReadRequestMsg = (MpiReadRequestMsg)msg_v;
    MpiStreamWSR StreamWSR = ReadRequestMsg->StreamWS;
    MpiStreamWS StreamWS = StreamWSR->StreamWS;
    CP_Services Svcs = (CP_Services)client_Data;

    Svcs->verbose(StreamWS->Stream.CP_Stream, DPTraceVerbose,
                  "MpiReadRequestHandler:"
                  "read request from reader=%d,ts=%d,off=%d,len=%d\n",
                  ReadRequestMsg->RequestingRank, ReadRequestMsg->TimeStep,
                  ReadRequestMsg->Offset, ReadRequestMsg->Length);

    PERFSTUBS_TIMER_START_FUNC(timer);

    char *outboundBuffer = NULL;
    if (NULL == (outboundBuffer = FetchTimeStep(
                     StreamWS->TimeSteps, ReadRequestMsg->TimeStep,
                     ReadRequestMsg->Offset, ReadRequestMsg->Length)))
    {
        PERFSTUBS_TIMER_STOP_FUNC(timer);
        return;
    }

    struct _MpiReadReplyMsg ReadReplyMsg = {
        .TimeStep = ReadRequestMsg->TimeStep,
        .DataLength = ReadRequestMsg->Length,
        .StreamRS = ReadRequestMsg->StreamRS,
        .NotifyCondition = ReadRequestMsg->NotifyCondition,
        .MpiPortName = StreamWSR->MpiPortName,
    };

    if (MPI_DP_LOCAL == ReadRequestMsg->CommType)
    {
        ReadReplyMsg.Data = outboundBuffer;
    }

    Svcs->verbose(
        StreamWS->Stream.CP_Stream, DPTraceVerbose,
        "MpiReadRequestHandler: Replying reader=%d with MPI port name=%s\n",
        ReadRequestMsg->RequestingRank, StreamWSR->MpiPortName);

    Svcs->sendToPeer(StreamWS->Stream.CP_Stream, StreamWSR->Link.PeerCohort,
                     ReadRequestMsg->RequestingRank, StreamWS->ReadReplyFormat,
                     &ReadReplyMsg);

    if (MPI_DP_REMOTE == ReadRequestMsg->CommType)
    {
        // Send the actual Data using MPI
        MPI_Comm *comm =
            &StreamWSR->ReaderContactInfo[ReadRequestMsg->RequestingRank]
                 .MpiComm;
        MPI_Errhandler worldErrHandler;
        MPI_Comm_get_errhandler(MPI_COMM_WORLD, &worldErrHandler);
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
        int ret = MPI_Send(outboundBuffer, ReadRequestMsg->Length, MPI_CHAR, 0,
                           ReadRequestMsg->NotifyCondition, *comm);
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, worldErrHandler);

        if (ret != MPI_SUCCESS)
        {
            MPI_Comm_accept(StreamWSR->MpiPortName, MPI_INFO_NULL, 0,
                            MPI_COMM_SELF, comm);
            Svcs->verbose(
                StreamWS->Stream.CP_Stream, DPTraceVerbose,
                "MpiReadRequestHandler: Accepted client, Link.CohortSize=%d\n",
                StreamWSR->Link.CohortSize);
            MPI_Send(outboundBuffer, ReadRequestMsg->Length, MPI_CHAR, 0,
                     ReadRequestMsg->NotifyCondition, *comm);
        }
    }

    free(outboundBuffer);

    PERFSTUBS_TIMER_STOP_FUNC(timer);
}

typedef struct _MpiCompletionHandle
{
    int CMcondition;
    CManager cm;
    void *CPStream;
    void *Buffer;
    int Rank;
    enum MPI_DP_COMM_TYPE CommType;
} * MpiCompletionHandle;

/**
 * This is invoked at the Reader side when a reply is ready to be read
 *
 */
static void MpiReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                void *client_Data, attr_list attrs)
{
    PERFSTUBS_TIMER_START_FUNC(timer);
    MpiReadReplyMsg ReadReplyMsg = (MpiReadReplyMsg)msg_v;
    MpiStreamRD StreamRS = ReadReplyMsg->StreamRS;
    CP_Services Svcs = (CP_Services)client_Data;
    MpiCompletionHandle Handle =
        CMCondition_get_client_data(cm, ReadReplyMsg->NotifyCondition);

    Svcs->verbose(
        StreamRS->Stream.CP_Stream, DPTraceVerbose,
        "MpiReadReplyHandler: Read recv from rank=%d,condition=%d,size=%d\n",
        Handle->Rank, ReadReplyMsg->NotifyCondition, ReadReplyMsg->DataLength);

    if (MPI_DP_LOCAL == Handle->CommType)
    {
        memcpy(Handle->Buffer, ReadReplyMsg->Data, ReadReplyMsg->DataLength);
    }
    else
    {
        pthread_rwlock_rdlock(&LockRS);
        MPI_Comm comm = StreamRS->WriterContactInfo[Handle->Rank].MpiComm;
        pthread_rwlock_unlock(&LockRS);

        MPI_Errhandler worldErrHandler;
        MPI_Comm_get_errhandler(MPI_COMM_WORLD, &worldErrHandler);
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
        int ret =
            MPI_Recv(Handle->Buffer, ReadReplyMsg->DataLength, MPI_CHAR, 0,
                     ReadReplyMsg->NotifyCondition, comm, MPI_STATUS_IGNORE);
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, worldErrHandler);

        if (ret != MPI_SUCCESS)
        {
            MPI_Comm_connect(ReadReplyMsg->MpiPortName, MPI_INFO_NULL, 0,
                             MPI_COMM_SELF, &comm);

            Svcs->verbose(StreamRS->Stream.CP_Stream, DPTraceVerbose,
                          "MpiReadReplyHandler: Connecting to MPI Server\n");
            MPI_Recv(Handle->Buffer, ReadReplyMsg->DataLength, MPI_CHAR, 0,
                     ReadReplyMsg->NotifyCondition, comm, MPI_STATUS_IGNORE);
        }

        pthread_rwlock_wrlock(&LockRS);
        StreamRS->WriterContactInfo[Handle->Rank].MpiComm = comm;
        pthread_rwlock_unlock(&LockRS);
    }
    StreamRS->Link.Stats->DataBytesReceived += ReadReplyMsg->DataLength;

    /*
     * Signal the condition to wake the reader if they are waiting.
     */
    CMCondition_signal(cm, ReadReplyMsg->NotifyCondition);
    PERFSTUBS_TIMER_STOP_FUNC(timer);
}

/*
 *
 *   InitWriter.    Called by the control plane collectively during the early
 * stages of Open on the writer side.  It should do whatever is necessary to
 * initialize a new writer-side data plane.  This does *not* include creating
 * contact information per se.  That can be put off until InitWriterPerReader().
 *
 */
static DP_WS_Stream MpiInitWriter(CP_Services Svcs, void *CP_Stream,
                                  struct _SstParams *Params, attr_list DPAttrs,
                                  SstStats Stats)
{
    pthread_once(&OnceMpiInitializer, MpiInitialize);

    /* Make MutexReaders to be recursive */

    MpiStreamWS Stream = calloc(sizeof(struct _MpiStreamWS), 1);
    CManager cm = Svcs->getCManager(CP_Stream);
    SMPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&Stream->MutexReaders, &attr);

    SMPI_Comm_rank(comm, &Stream->Stream.Rank);

    Stream->Stream.CP_Stream = CP_Stream;
    Stream->Stream.PID = getpid();

    /*
     * add a handler for read request messages
     */
    F = CMregister_format(cm, MpiReadRequestStructs);
    CMregister_handler(F, MpiReadRequestHandler, Svcs);

    /*
     * register read reply message structure so we can send later
     */
    Stream->ReadReplyFormat = CMregister_format(cm, MpiReadReplyStructs);

    return (void *)Stream;
}

/**
 * InitWriterPerReader.
 *
 * Called by the control plane collectively when accepting a new reader
 * connection.  It receives the per-rank reader contact information (as created
 * on the connecting peer in InitReader) and should create its own
 * per-writer-rank contact information and place it in *writerContactInfoPtr.
 * The structure of that information should be described by
 * DPInterface.WriterContactFormats.   (This is an FFS format description.  See
 * https://www.cc.gatech.edu/systems/projects/FFS/.)
 *
 */
static DP_WSR_Stream
MpiInitWriterPerReader(CP_Services Svcs, DP_WS_Stream WS_Stream_v,
                       int readerCohortSize, CP_PeerCohort PeerCohort,
                       void **providedReaderInfo_v, void **WriterContactInfoPtr)
{
    MpiStreamWS StreamWS = (MpiStreamWS)WS_Stream_v;
    MpiStreamWSR StreamWSR = calloc(sizeof(struct _MpiStreamWSR), 1);
    MpiWriterContactInfo ContactInfo;
    SMPI_Comm comm = Svcs->getMPIComm(StreamWS->Stream.CP_Stream);
    MpiReaderContactInfo *providedReaderInfo =
        (MpiReaderContactInfo *)providedReaderInfo_v;
    char *MpiContactString = calloc(sizeof(char), MPI_DP_CONTACT_STRING_LEN);

    int Rank;
    SMPI_Comm_rank(comm, &Rank);
    snprintf(MpiContactString, MPI_DP_CONTACT_STRING_LEN,
             "Writer Rank %d, test contact", Rank);

    MPI_Open_port(MPI_INFO_NULL, StreamWSR->MpiPortName);

    StreamWSR->StreamWS = StreamWS; /* pointer to writer struct */
    StreamWSR->Link.PeerCohort = PeerCohort;
    StreamWSR->Link.CohortSize = readerCohortSize;

    Svcs->verbose(StreamWS->Stream.CP_Stream, DPTraceVerbose,
                  "MPI dataplane WriterPerReader to be initialized\n");

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    StreamWSR->ReaderContactInfo =
        calloc(sizeof(struct _MpiReaderContactInfo), readerCohortSize);
    for (int i = 0; i < readerCohortSize; i++)
    {
        StreamWSR->ReaderContactInfo[i].ContactString =
            strdup(providedReaderInfo[i]->ContactString);
        StreamWSR->ReaderContactInfo[i].StreamRS =
            providedReaderInfo[i]->StreamRS;
        StreamWSR->ReaderContactInfo[i].MpiComm = MPI_COMM_NULL;
        Svcs->verbose(
            StreamWS->Stream.CP_Stream, DPTraceVerbose,
            "Received contact info \"%s\", RD_Stream %p for Reader Rank %d\n",
            StreamWSR->ReaderContactInfo[i].ContactString,
            StreamWSR->ReaderContactInfo[i].StreamRS, i);
    }

    /*
     * add this writer-side reader-specific stream to the parent writer stream
     * structure
     */
    pthread_mutex_lock(&StreamWS->MutexReaders);
    StreamWS->Readers = realloc(
        StreamWS->Readers, sizeof(*StreamWSR) * (StreamWS->ReaderCount + 1));
    StreamWS->Readers[StreamWS->ReaderCount] = StreamWSR;
    StreamWS->ReaderCount++;
    pthread_mutex_unlock(&StreamWS->MutexReaders);

    ContactInfo = calloc(sizeof(struct _MpiWriterContactInfo), 1);
    ContactInfo->ContactString = MpiContactString;
    ContactInfo->StreamWS = StreamWSR;
    ContactInfo->PID = StreamWS->Stream.PID;
    *WriterContactInfoPtr = ContactInfo;

    return StreamWSR;
}

static void MpiProvideWriterDataToReader(CP_Services Svcs,
                                         DP_RS_Stream RS_Stream_v,
                                         int writerCohortSize,
                                         CP_PeerCohort PeerCohort,
                                         void **providedWriterInfo_v)
{
    MpiStreamRD StreamRS = (MpiStreamRD)RS_Stream_v;
    MpiWriterContactInfo *providedWriterInfo =
        (MpiWriterContactInfo *)providedWriterInfo_v;

    StreamRS->Link.PeerCohort = PeerCohort;
    StreamRS->Link.CohortSize = writerCohortSize;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    struct _MpiWriterContactInfo *tmp =
        calloc(sizeof(struct _MpiWriterContactInfo), writerCohortSize);
    for (int i = 0; i < writerCohortSize; i++)
    {
        tmp[i].ContactString = strdup(providedWriterInfo[i]->ContactString);
        tmp[i].StreamWS = providedWriterInfo[i]->StreamWS;
        tmp[i].MpiComm = MPI_COMM_NULL;
        tmp[i].PID = providedWriterInfo[i]->PID;

        if (StreamRS->WriterContactInfo &&
            StreamRS->WriterContactInfo[i].MpiComm != MPI_COMM_NULL)
        {
            MPI_Comm_disconnect(&StreamRS->WriterContactInfo[i].MpiComm);
        }

        Svcs->verbose(StreamRS->Stream.CP_Stream, DPTraceVerbose,
                      "Received contact info \"%s\", WS_stream %p for WSR Rank "
                      "%d\n",
                      tmp[i].ContactString, tmp[i].StreamWS, i);
    }

    StreamRS->WriterContactInfo = tmp;
}

/**
 * ReadRemoteMemory.
 *
 * Called by the control plane on the reader side to request that timestep data
 * from the writer side, identified by Rank, TimeStep, starting at a particular
 * Offset and continuing for Length, be placed into a local Buffer.  The
 * DP_TimeStepInfo value will be the per-rank per-timestep information that was
 * created during ProvideTimeStep by that writer rank for that timestep.
 * This is an asyncronous request in that it need not be completed before this
 * call returns.  The void* return value will later be passed to a
 * WaitForCompletion call and should represent a completion handle.
 *
 */
static void *MpiReadRemoteMemory(CP_Services Svcs, DP_RS_Stream Stream_v,
                                 int Rank, long TimeStep, size_t Offset,
                                 size_t Length, void *Buffer,
                                 void *DP_TimeStepInfo)
{
    /* DP_RS_Stream is the return from InitReader */
    MpiStreamRD Stream = (MpiStreamRD)Stream_v;
    CManager cm = Svcs->getCManager(Stream->Stream.CP_Stream);
    MpiCompletionHandle ret = calloc(sizeof(struct _MpiCompletionHandle), 1);

    pthread_rwlock_rdlock(&LockRS);

    ret->CMcondition = CMCondition_get(cm, NULL);
    ret->CPStream = Stream->Stream.CP_Stream;
    ret->cm = cm;
    ret->Buffer = Buffer;
    ret->Rank = Rank;
    ret->CommType = (Stream->WriterContactInfo[Rank].PID == Stream->Stream.PID)
                        ? MPI_DP_LOCAL
                        : MPI_DP_REMOTE;

    /*
     * set the completion handle as client Data on the condition so that
     * handler has access to it.
     */
    CMCondition_set_client_data(cm, ret->CMcondition, ret);

    Svcs->verbose(
        Stream->Stream.CP_Stream, DPTraceVerbose,
        "Reader (rank %d) requesting to read remote memory for TimeStep %d "
        "from Rank %d, StreamWSR = %p,  Offset=%d, Length=%d\n",
        Stream->Stream.Rank, TimeStep, Rank,
        Stream->WriterContactInfo[Rank].StreamWS, Offset, Length);

    /* send request to appropriate writer */
    struct _MpiReadRequestMsg ReadRequestMsg = {
        .TimeStep = TimeStep,
        .Offset = Offset,
        .Length = Length,
        .StreamWS = Stream->WriterContactInfo[Rank].StreamWS,
        .StreamRS = Stream,
        .RequestingRank = Stream->Stream.Rank,
        .NotifyCondition = ret->CMcondition,
        .CommType = ret->CommType};

    pthread_rwlock_unlock(&LockRS);

    Svcs->sendToPeer(Stream->Stream.CP_Stream, Stream->Link.PeerCohort, Rank,
                     Stream->ReadRequestFormat, &ReadRequestMsg);
    return ret;
}

/**
 * WaitForCompletion.
 *
 * Called by the control plane on the reader side with a Handle that is the
 * return value of a prior ReadRemoteMemory call. This call should not return
 * until that particular remote memory read is complete and the buffer is full.
 * A zero return means that the read failed and will result in a (hopefully
 * orderly) shutdown of the stream.
 */
static int MpiWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    MpiCompletionHandle Handle = (MpiCompletionHandle)Handle_v;
    Svcs->verbose(
        Handle->CPStream, DPTraceVerbose,
        "Waiting for completion of memory read to rank %d, condition %d\n",
        Handle->Rank, Handle->CMcondition);

    /*
     * Wait for the CM condition to be signalled.  If it has been already,
     * this returns immediately.  Copying the incoming data to the waiting
     * buffer has been done by the reply handler.
     */
    int Ret = CMCondition_wait(Handle->cm, Handle->CMcondition);
    if (!Ret)
    {
        Svcs->verbose(Handle->CPStream, DPTraceVerbose,
                      "Remote memory read to rank %d with "
                      "condition %d has FAILED because of "
                      "writer failure\n",
                      Handle->Rank, Handle->CMcondition);
    }
    else
    {
        if (Handle->CMcondition != -1)
            Svcs->verbose(Handle->CPStream, DPTraceVerbose,
                          "Remote memory read to rank %d with condition %d has "
                          "completed\n",
                          Handle->Rank, Handle->CMcondition);
    }
    free(Handle);
    return Ret;
}

/**
 * ProvideTimeStep.
 *
 * Called by the control plane collectively on the writer side to "give" the
 * data plane new data that is should then serve to the readers.  DP must do
 * everything necessary here to allow future service (until ReleaseTimeStep is
 * called).  The DP can create per-timestep per-rank identifying information
 * that will be placed in the timestep metadata and provided to the reader
 * during remote read requests.  A pointer to this contact information should
 * be placed in *TimeStepInfoPtr.  This structure should be described by
 * DPInterface.TimeStepInfoFormats.
 *
 */
static void MpiProvideTimeStep(CP_Services Svcs, DP_WS_Stream Stream_v,
                               struct _SstData *Data,
                               struct _SstData *LocalMetadata, long TimeStep,
                               void **TimeStepInfoPtr)
{
    MpiStreamWS Stream = (MpiStreamWS)Stream_v;
    TimeStepList Entry = calloc(sizeof(struct _TimeStepEntry), 1);
    struct _MpiPerTimeStepInfo *Info =
        calloc(sizeof(struct _MpiPerTimeStepInfo), 1);

    Info->CheckString = calloc(sizeof(char), MPI_DP_CONTACT_STRING_LEN);
    snprintf(Info->CheckString, MPI_DP_CONTACT_STRING_LEN,
             "Mpi info for timestep %ld from rank %d", TimeStep,
             Stream->Stream.Rank);

    Entry->Data = malloc(sizeof(struct _SstData));
    memcpy(Entry->Data, Data, sizeof(struct _SstData));
    Entry->TimeStep = TimeStep;
    Entry->DP_TimeStepInfo = Info;

    pthread_rwlock_wrlock(&LockTS);
    Entry->Next = Stream->TimeSteps;
    Stream->TimeSteps = Entry;
    pthread_rwlock_unlock(&LockTS);
    *TimeStepInfoPtr = Info;
}

/**
 * ReleaseTimeStep.
 *
 * Called by the control plane collectively on the writer side to tell the data
 * plane that a particular timestep is no longer required and any resources
 * devoted to serving it can be released.
 */
static void MpiReleaseTimeStep(CP_Services Svcs, DP_WS_Stream Stream_v,
                               long TimeStep)
{
    MpiStreamWS Stream = (MpiStreamWS)Stream_v;
    TimeStepList List = Stream->TimeSteps;

    Svcs->verbose(Stream->Stream.CP_Stream, DPTraceVerbose,
                  "Releasing timestep %ld\n", TimeStep);

    pthread_rwlock_wrlock(&LockTS);
    if (Stream->TimeSteps->TimeStep == TimeStep)
    {
        Stream->TimeSteps = List->Next;
        free(List->Data);
        free(List);
    }
    else
    {
        TimeStepList last = List;
        List = List->Next;
        while (List != NULL)
        {
            if (List->TimeStep == TimeStep)
            {
                last->Next = List->Next;
                free(List->Data);
                free(List);
                pthread_rwlock_unlock(&LockTS);
                return;
            }
            last = List;
            List = List->Next;
        }
        /*
         * Shouldn't ever get here because we should never release a
         * timestep that we don't have.
         */
        fprintf(stderr, "Failed to release TimeStep %ld, not found\n",
                TimeStep);
    }
    pthread_rwlock_unlock(&LockTS);
}

static int MpiGetPriority(CP_Services Svcs, void *CP_Stream,
                          struct _SstParams *Params)
{
#if defined(MPICH)
    // Only enabled when MPI_THREAD_MULTIPLE and using MPICH
    int provided = 0;
    pthread_once(&OnceMpiInitializer, MpiInitialize);
    MPI_Query_thread(&provided);
    if (provided == MPI_THREAD_MULTIPLE)
    {
        return 100;
    }
#endif
    return -100;
}

static void MpiNotifyConnFailure(CP_Services Svcs, DP_RS_Stream Stream_v,
                                 int FailedPeerRank)
{
    /* DP_RS_Stream is the return from InitReader */
    MpiStreamRD Stream = (MpiStreamRD)Stream_v;
    Svcs->verbose(Stream->Stream.CP_Stream, DPTraceVerbose,
                  "received notification that writer peer "
                  "%d has failed, failing any pending "
                  "requests\n",
                  FailedPeerRank);
}

static void MpiDestroyWriterPerReader(CP_Services Svcs,
                                      DP_WSR_Stream WSR_Stream_v)
{
    MpiStreamWSR StreamWSR = (MpiStreamWSR)WSR_Stream_v;
    MpiStreamWS StreamWS = StreamWSR->StreamWS;

    char MpiPortName[MPI_MAX_PORT_NAME] = {0};
    const int CohortSize = StreamWSR->Link.CohortSize;
    MPI_Comm *Comms_to_disconnect = calloc(sizeof(MPI_Comm), CohortSize);

    pthread_mutex_lock(&StreamWS->MutexReaders);
    strncpy(MpiPortName, StreamWSR->MpiPortName, MPI_MAX_PORT_NAME);

    for (int i = 0; i < CohortSize; i++)
    {
        Comms_to_disconnect[i] = StreamWSR->ReaderContactInfo[i].MpiComm;
        if (StreamWSR->ReaderContactInfo[i].ContactString)
        {
            free(StreamWSR->ReaderContactInfo[i].ContactString);
        }
    }

    if (StreamWSR->ReaderContactInfo)
    {
        free(StreamWSR->ReaderContactInfo);
    }

    StreamWSR->Link.CohortSize = 0;

    for (int i = 0; i < StreamWS->ReaderCount; i++)
    {
        if (StreamWS->Readers[i] == StreamWSR)
        {
            StreamWS->Readers[i] = StreamWS->Readers[StreamWS->ReaderCount - 1];

            StreamWS->Readers =
                realloc(StreamWS->Readers,
                        sizeof(*StreamWSR) * (StreamWS->ReaderCount - 1));
            StreamWS->ReaderCount--;
            break;
        }
    }

    free(StreamWSR);
    pthread_mutex_unlock(&StreamWS->MutexReaders);

    // MPI routines must be called outsie of a critical region
    for (int i = 0; i < CohortSize; i++)
    {
        if (Comms_to_disconnect[i] != MPI_COMM_NULL)
        {
            MPI_Comm_disconnect(&Comms_to_disconnect[i]);
        }
    }
    free(Comms_to_disconnect);

    MPI_Close_port(MpiPortName);
}

static void MpiDestroyWriter(CP_Services Svcs, DP_WS_Stream WS_Stream_v)
{
    MpiStreamWS StreamWS = (MpiStreamWS)WS_Stream_v;

    pthread_mutex_lock(&StreamWS->MutexReaders);
    while (StreamWS->ReaderCount)
    {
        MpiDestroyWriterPerReader(Svcs,
                                  StreamWS->Readers[StreamWS->ReaderCount - 1]);
    }

    pthread_mutex_t *mutex_to_delete = &StreamWS->MutexReaders;
    free(StreamWS->Readers);
    free(StreamWS);
    pthread_mutex_unlock(mutex_to_delete);
}

static void MpiDestroyReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v)
{
    MpiStreamRD StreamRS = (MpiStreamRD)RS_Stream_v;
    const int CohortSize = StreamRS->Link.CohortSize;
    MPI_Comm *MpiComms = calloc(sizeof(MPI_Comm), CohortSize);

    pthread_rwlock_wrlock(&LockRS);

    for (int i = 0; i < CohortSize; i++)
    {
        MpiComms[i] = StreamRS->WriterContactInfo[i].MpiComm;
        free(StreamRS->WriterContactInfo[i].ContactString);
    }
    free(StreamRS->WriterContactInfo);
    free(StreamRS);

    pthread_rwlock_unlock(&LockRS);

    for (int i = 0; i < CohortSize; i++)
    {
        if (MpiComms[i] != MPI_COMM_NULL)
        {
            MPI_Comm_disconnect(&MpiComms[i]);
        }
    }
    free(MpiComms);
}

extern CP_DP_Interface LoadMpiDP()
{
    static struct _CP_DP_Interface mpiDPInterface = {
        .ReaderContactFormats = MpiReaderContactStructs,
        .WriterContactFormats = MpiWriterContactStructs,
        .initReader = MpiInitReader,
        .initWriter = MpiInitWriter,
        .initWriterPerReader = MpiInitWriterPerReader,
        .provideWriterDataToReader = MpiProvideWriterDataToReader,
        .readRemoteMemory = MpiReadRemoteMemory,
        .waitForCompletion = MpiWaitForCompletion,
        .provideTimestep = MpiProvideTimeStep,
        .releaseTimestep = MpiReleaseTimeStep,
        .getPriority = MpiGetPriority,
        .destroyReader = MpiDestroyReader,
        .destroyWriter = MpiDestroyWriter,
        .destroyWriterPerReader = MpiDestroyWriterPerReader,
        .notifyConnFailure = MpiNotifyConnFailure,
    };

    return &mpiDPInterface;
}
