#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atl.h>
#include <evpath.h>

#include "sst_data.h"

#include "adios2/toolkit/profiling/taustubs/taustubs.h"
#include "dp_interface.h"

/*
 *  Some conventions:
 *    `RS` indicates a reader-side item.
 *    `WS` indicates a writer-side item.
 *    `WSR` indicates a writer-side per-reader item.
 *
 *   We keep different "stream" structures for the reader side and for the
 *   writer side.  On the writer side, there's actually a "stream"
 *   per-connected-reader (a WSR_Stream), with the idea that some (many?)
 *   RDMA transports will require connections/pairing, so we'd need to track
 *   resources per reader.
 *
 *   Generally the 'contact information' we exchange at init time includes
 *   the address of the local 'stream' data structure.  This address isn't
 *   particularly useful to the far side, but it can be returned with
 *   requests to indicate what resource is targeted.  For example, when a
 *   remote memory read request arrives at the writer from the reader, it
 *   includes the WSR_Stream value that is the address of the writer-side
 *   per-reader data structure.  Upon message arrival, we just cast that
 *   value back into a pointer.
 *
 *   By design, neither the data plane nor the control plane reference the
 *   other's symbols directly.  The interface between the control plane and
 *   the data plane is represented by the types and structures defined in
 *   dp_interface.h and is a set of function pointers and FFS-style
 *   descriptions of the data structures to be communicated at init time.
 *   This allows for the future possibility of loading planes at run-time, etc.
 *
 *   This "dummy" data plane uses control plane functionality to implement
 *   the ReadRemoteMemory functionality.  That is, it both the request to
 *   read memory and the response which carries the data are actually
 *   accomplished using the connections and message delivery facilities of
 *   the control plane, made available here via CP_Services.  A real data
 *   plane would replace one or both of these with RDMA functionality.
 */

typedef struct _Dummy_RS_Stream
{
    CManager cm;
    void *CP_Stream;
    CMFormat ReadRequestFormat;
    int Rank;

    /* writer info */
    int WriterCohortSize;
    CP_PeerCohort PeerCohort;
    struct _DummyWriterContactInfo *WriterContactInfo;
} * Dummy_RS_Stream;

typedef struct _Dummy_WSR_Stream
{
    struct _Dummy_WS_Stream *WS_Stream;
    CP_PeerCohort PeerCohort;
    int ReaderCohortSize;
    struct _DummyReaderContactInfo *ReaderContactInfo;
} * Dummy_WSR_Stream;

typedef struct _TimestepEntry
{
    long Timestep;
    struct _SstData *Data;
    struct _DummyPerTimestepInfo *DP_TimestepInfo;
    struct _TimestepEntry *Next;

} * TimestepList;

typedef struct _Dummy_WS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;

    TimestepList Timesteps;
    CMFormat ReadReplyFormat;

    int ReaderCount;
    Dummy_WSR_Stream *Readers;
} * Dummy_WS_Stream;

typedef struct _DummyReaderContactInfo
{
    char *ContactString;
    void *RS_Stream;
} * DummyReaderContactInfo;

typedef struct _DummyWriterContactInfo
{
    char *ContactString;
    void *WS_Stream;
} * DummyWriterContactInfo;

typedef struct _DummyReadRequestMsg
{
    long Timestep;
    size_t Offset;
    size_t Length;
    void *WS_Stream;
    void *RS_Stream;
    int RequestingRank;
    int NotifyCondition;
} * DummyReadRequestMsg;

static FMField DummyReadRequestList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(DummyReadRequestMsg, Timestep)},
    {"Offset", "integer", sizeof(size_t),
     FMOffset(DummyReadRequestMsg, Offset)},
    {"Length", "integer", sizeof(size_t),
     FMOffset(DummyReadRequestMsg, Length)},
    {"WS_Stream", "integer", sizeof(void *),
     FMOffset(DummyReadRequestMsg, WS_Stream)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(DummyReadRequestMsg, RS_Stream)},
    {"RequestingRank", "integer", sizeof(int),
     FMOffset(DummyReadRequestMsg, RequestingRank)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(DummyReadRequestMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec DummyReadRequestStructs[] = {
    {"DummyReadRequest", DummyReadRequestList,
     sizeof(struct _DummyReadRequestMsg), NULL},
    {NULL, NULL, 0, NULL}};

typedef struct _DummyReadReplyMsg
{
    long Timestep;
    size_t DataLength;
    void *RS_Stream;
    char *Data;
    int NotifyCondition;
} * DummyReadReplyMsg;

static FMField DummyReadReplyList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(DummyReadReplyMsg, Timestep)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(DummyReadReplyMsg, RS_Stream)},
    {"DataLength", "integer", sizeof(size_t),
     FMOffset(DummyReadReplyMsg, DataLength)},
    {"Data", "char[DataLength]", sizeof(char),
     FMOffset(DummyReadReplyMsg, Data)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(DummyReadReplyMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec DummyReadReplyStructs[] = {
    {"DummyReadReply", DummyReadReplyList, sizeof(struct _DummyReadReplyMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static void DummyReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                  void *client_Data, attr_list attrs);

static DP_RS_Stream DummyInitReader(CP_Services Svcs, void *CP_Stream,
                                    void **ReaderContactInfoPtr)
{
    Dummy_RS_Stream Stream = malloc(sizeof(struct _Dummy_RS_Stream));
    DummyReaderContactInfo Contact =
        malloc(sizeof(struct _DummyReaderContactInfo));
    CManager cm = Svcs->getCManager(CP_Stream);
    char *DummyContactString = malloc(64);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    memset(Stream, 0, sizeof(*Stream));
    memset(Contact, 0, sizeof(*Contact));

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    MPI_Comm_rank(comm, &Stream->Rank);

    sprintf(DummyContactString, "Reader Rank %d, test contact", Stream->Rank);

    /*
     * add a handler for read reply messages
     */
    Stream->ReadRequestFormat = CMregister_format(cm, DummyReadRequestStructs);
    F = CMregister_format(cm, DummyReadReplyStructs);
    CMregister_handler(F, DummyReadReplyHandler, Svcs);

    Contact->ContactString = DummyContactString;
    Contact->RS_Stream = Stream;

    *ReaderContactInfoPtr = Contact;

    return Stream;
}

static void DummyReadRequestHandler(CManager cm, CMConnection conn, void *msg_v,
                                    void *client_Data, attr_list attrs)
{
    TAU_START_FUNC();
    DummyReadRequestMsg ReadRequestMsg = (DummyReadRequestMsg)msg_v;
    Dummy_WSR_Stream WSR_Stream = ReadRequestMsg->WS_Stream;

    Dummy_WS_Stream WS_Stream = WSR_Stream->WS_Stream;
    TimestepList tmp = WS_Stream->Timesteps;
    CP_Services Svcs = (CP_Services)client_Data;

    Svcs->verbose(WS_Stream->CP_Stream,
                  "Got a request to read remote memory "
                  "from reader rank %d: timestep %d, "
                  "offset %d, length %d\n",
                  ReadRequestMsg->RequestingRank, ReadRequestMsg->Timestep,
                  ReadRequestMsg->Offset, ReadRequestMsg->Length);
    while (tmp != NULL)
    {
        if (tmp->Timestep == ReadRequestMsg->Timestep)
        {
            struct _DummyReadReplyMsg ReadReplyMsg;
            /* memset avoids uninit byte warnings from valgrind */
            memset(&ReadReplyMsg, 0, sizeof(ReadReplyMsg));
            ReadReplyMsg.Timestep = ReadRequestMsg->Timestep;
            ReadReplyMsg.DataLength = ReadRequestMsg->Length;
            ReadReplyMsg.Data = tmp->Data->block + ReadRequestMsg->Offset;
            ReadReplyMsg.RS_Stream = ReadRequestMsg->RS_Stream;
            ReadReplyMsg.NotifyCondition = ReadRequestMsg->NotifyCondition;
            Svcs->verbose(
                WS_Stream->CP_Stream,
                "Sending a reply to reader rank %d for remote memory read\n",
                ReadRequestMsg->RequestingRank);
            Svcs->sendToPeer(WS_Stream->CP_Stream, WSR_Stream->PeerCohort,
                             ReadRequestMsg->RequestingRank,
                             WS_Stream->ReadReplyFormat, &ReadReplyMsg);
            TAU_STOP_FUNC();
            return;
        }
        tmp = tmp->Next;
    }
    /*
     * Shouldn't ever get here because we should never get a request for a
     * timestep that we don't have.
     */
    fprintf(stderr, "Failed to read Timestep %ld, not found\n",
            ReadRequestMsg->Timestep);
    /*
     * in the interest of not failing a writer on a reader failure, don't
     * assert(0) here.  Probably this sort of error should close the link to
     * a reader though.
     */
    TAU_STOP_FUNC();
}

typedef struct _DummyCompletionHandle
{
    int CMcondition;
    CManager cm;
    void *CPStream;
    void *Buffer;
    int Rank;
} * DummyCompletionHandle;

static void DummyReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                  void *client_Data, attr_list attrs)
{
    TAU_START_FUNC();
    DummyReadReplyMsg ReadReplyMsg = (DummyReadReplyMsg)msg_v;
    Dummy_RS_Stream RS_Stream = ReadReplyMsg->RS_Stream;
    CP_Services Svcs = (CP_Services)client_Data;
    DummyCompletionHandle Handle =
        CMCondition_get_client_data(cm, ReadReplyMsg->NotifyCondition);

    Svcs->verbose(
        RS_Stream->CP_Stream,
        "Got a reply to remote memory read from rank %d, condition is %d\n",
        Handle->Rank, ReadReplyMsg->NotifyCondition);

    /*
     * `Handle` contains the full request info and is `client_data`
     * associated with the CMCondition.  Once we get it, copy the incoming
     * data to the buffer area given by the request
     */
    memcpy(Handle->Buffer, ReadReplyMsg->Data, ReadReplyMsg->DataLength);

    /*
     * Signal the condition to wake the reader if they are waiting.
     */
    CMCondition_signal(cm, ReadReplyMsg->NotifyCondition);
    TAU_STOP_FUNC();
}

static DP_WS_Stream DummyInitWriter(CP_Services Svcs, void *CP_Stream)
{
    Dummy_WS_Stream Stream = malloc(sizeof(struct _Dummy_WS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    memset(Stream, 0, sizeof(struct _Dummy_WS_Stream));

    MPI_Comm_rank(comm, &Stream->Rank);

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    /*
     * add a handler for read request messages
     */
    F = CMregister_format(cm, DummyReadRequestStructs);
    CMregister_handler(F, DummyReadRequestHandler, Svcs);

    /*
     * register read reply message structure so we can send later
     */
    Stream->ReadReplyFormat = CMregister_format(cm, DummyReadReplyStructs);

    return (void *)Stream;
}

static DP_WSR_Stream DummyInitWriterPerReader(CP_Services Svcs,
                                              DP_WS_Stream WS_Stream_v,
                                              int readerCohortSize,
                                              CP_PeerCohort PeerCohort,
                                              void **providedReaderInfo_v,
                                              void **WriterContactInfoPtr)
{
    Dummy_WS_Stream WS_Stream = (Dummy_WS_Stream)WS_Stream_v;
    Dummy_WSR_Stream WSR_Stream = malloc(sizeof(*WSR_Stream));
    DummyWriterContactInfo ContactInfo;
    MPI_Comm comm = Svcs->getMPIComm(WS_Stream->CP_Stream);
    int Rank;
    char *DummyContactString = malloc(64);
    DummyReaderContactInfo *providedReaderInfo =
        (DummyReaderContactInfo *)providedReaderInfo_v;

    MPI_Comm_rank(comm, &Rank);
    sprintf(DummyContactString, "Writer Rank %d, test contact", Rank);

    WSR_Stream->WS_Stream = WS_Stream; /* pointer to writer struct */
    WSR_Stream->PeerCohort = PeerCohort;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    WSR_Stream->ReaderContactInfo =
        malloc(sizeof(struct _DummyReaderContactInfo) * readerCohortSize);
    for (int i = 0; i < readerCohortSize; i++)
    {
        WSR_Stream->ReaderContactInfo[i].ContactString =
            strdup(providedReaderInfo[i]->ContactString);
        WSR_Stream->ReaderContactInfo[i].RS_Stream =
            providedReaderInfo[i]->RS_Stream;
        Svcs->verbose(
            WS_Stream->CP_Stream,
            "Received contact info \"%s\", RD_Stream %p for Reader Rank %d\n",
            WSR_Stream->ReaderContactInfo[i].ContactString,
            WSR_Stream->ReaderContactInfo[i].RS_Stream, i);
    }

    /*
     * add this writer-side reader-specific stream to the parent writer stream
     * structure
     */
    WS_Stream->Readers = realloc(
        WS_Stream->Readers, sizeof(*WSR_Stream) * (WS_Stream->ReaderCount + 1));
    WS_Stream->Readers[WS_Stream->ReaderCount] = WSR_Stream;
    WS_Stream->ReaderCount++;

    ContactInfo = malloc(sizeof(struct _DummyWriterContactInfo));
    memset(ContactInfo, 0, sizeof(struct _DummyWriterContactInfo));
    ContactInfo->ContactString = DummyContactString;
    ContactInfo->WS_Stream = WSR_Stream;
    *WriterContactInfoPtr = ContactInfo;

    return WSR_Stream;
}

static void DummyProvideWriterDataToReader(CP_Services Svcs,
                                           DP_RS_Stream RS_Stream_v,
                                           int writerCohortSize,
                                           CP_PeerCohort PeerCohort,
                                           void **providedWriterInfo_v)
{
    Dummy_RS_Stream RS_Stream = (Dummy_RS_Stream)RS_Stream_v;
    DummyWriterContactInfo *providedWriterInfo =
        (DummyWriterContactInfo *)providedWriterInfo_v;

    RS_Stream->PeerCohort = PeerCohort;
    RS_Stream->WriterCohortSize = writerCohortSize;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    RS_Stream->WriterContactInfo =
        malloc(sizeof(struct _DummyWriterContactInfo) * writerCohortSize);
    for (int i = 0; i < writerCohortSize; i++)
    {
        RS_Stream->WriterContactInfo[i].ContactString =
            strdup(providedWriterInfo[i]->ContactString);
        RS_Stream->WriterContactInfo[i].WS_Stream =
            providedWriterInfo[i]->WS_Stream;
        Svcs->verbose(
            RS_Stream->CP_Stream,
            "Received contact info \"%s\", WS_stream %p for WSR Rank %d\n",
            RS_Stream->WriterContactInfo[i].ContactString,
            RS_Stream->WriterContactInfo[i].WS_Stream, i);
    }
}

typedef struct _DummyPerTimestepInfo
{
    char *CheckString;
    int CheckInt;
} * DummyPerTimestepInfo;

static void *DummyReadRemoteMemory(CP_Services Svcs, DP_RS_Stream Stream_v,
                                   int Rank, long Timestep, size_t Offset,
                                   size_t Length, void *Buffer,
                                   void *DP_TimestepInfo)
{
    Dummy_RS_Stream Stream = (Dummy_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    DummyCompletionHandle ret = malloc(sizeof(struct _DummyCompletionHandle));
    struct _DummyReadRequestMsg ReadRequestMsg;

    ret->CMcondition = CMCondition_get(cm, NULL);
    ret->CPStream = Stream->CP_Stream;
    ret->cm = cm;
    ret->Buffer = Buffer;
    ret->Rank = Rank;
    /*
     * set the completion handle as client Data on the condition so that
     * handler has access to it.
     */
    CMCondition_set_client_data(cm, ret->CMcondition, ret);

    Svcs->verbose(Stream->CP_Stream,
                  "Adios requesting to read remote memory for Timestep %d "
                  "from Rank %d, WSR_Stream = %p\n",
                  Timestep, Rank, Stream->WriterContactInfo[Rank].WS_Stream);

    /* send request to appropriate writer */
    /* memset avoids uninit byte warnings from valgrind */
    memset(&ReadRequestMsg, 0, sizeof(ReadRequestMsg));
    ReadRequestMsg.Timestep = Timestep;
    ReadRequestMsg.Offset = Offset;
    ReadRequestMsg.Length = Length;
    ReadRequestMsg.WS_Stream = Stream->WriterContactInfo[Rank].WS_Stream;
    ReadRequestMsg.RS_Stream = Stream;
    ReadRequestMsg.RequestingRank = Stream->Rank;
    ReadRequestMsg.NotifyCondition = ret->CMcondition;
    Svcs->sendToPeer(Stream->CP_Stream, Stream->PeerCohort, Rank,
                     Stream->ReadRequestFormat, &ReadRequestMsg);

    return ret;
}

static void DummyWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    DummyCompletionHandle Handle = (DummyCompletionHandle)Handle_v;
    Svcs->verbose(
        Handle->CPStream,
        "Waiting for completion of memory read to rank %d, condition %d\n",
        Handle->Rank, Handle->CMcondition);
    /*
     * Wait for the CM condition to be signalled.  If it has been already,
     * this returns immediately.  Copying the incoming data to the waiting
     * buffer has been done by the reply handler.
     */
    CMCondition_wait(Handle->cm, Handle->CMcondition);
    Svcs->verbose(
        Handle->CPStream,
        "Remote memory read to rank %d with condition %d has completed\n",
        Handle->Rank, Handle->CMcondition);
    free(Handle);
}

static void DummyProvideTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                 struct _SstData *Data,
                                 struct _SstData *LocalMetadata, long Timestep,
                                 void **TimestepInfoPtr)
{
    Dummy_WS_Stream Stream = (Dummy_WS_Stream)Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));
    struct _DummyPerTimestepInfo *Info =
        malloc(sizeof(struct _DummyPerTimestepInfo));

    Info->CheckString = malloc(64);
    sprintf(Info->CheckString, "Dummy info for timestep %ld from rank %d",
            Timestep, Stream->Rank);
    Info->CheckInt = Stream->Rank * 1000 + Timestep;
    Entry->Data = Data;
    Entry->Timestep = Timestep;
    Entry->DP_TimestepInfo = Info;

    Entry->Next = Stream->Timesteps;
    Stream->Timesteps = Entry;
    *TimestepInfoPtr = Info;
}

static void DummyReleaseTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                 long Timestep)
{
    Dummy_WS_Stream Stream = (Dummy_WS_Stream)Stream_v;
    TimestepList List = Stream->Timesteps;

    Svcs->verbose(Stream->CP_Stream, "Releasing timestep %ld\n", Timestep);
    if (Stream->Timesteps->Timestep == Timestep)
    {
        Stream->Timesteps = List->Next;
        free(List);
    }
    else
    {
        TimestepList last = List;
        List = List->Next;
        while (List != NULL)
        {
            if (List->Timestep == Timestep)
            {
                last->Next = List->Next;
                free(List);
                return;
            }
            last = List;
            List = List->Next;
        }
        /*
         * Shouldn't ever get here because we should never release a
         * timestep that we don't have.
         */
        fprintf(stderr, "Failed to release Timestep %ld, not found\n",
                Timestep);
        assert(0);
    }
}

static FMField DummyReaderContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(DummyReaderContactInfo, ContactString)},
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(DummyReaderContactInfo, RS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec DummyReaderContactStructs[] = {
    {"DummyReaderContactInfo", DummyReaderContactList,
     sizeof(struct _DummyReaderContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField DummyWriterContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(DummyWriterContactInfo, ContactString)},
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(DummyWriterContactInfo, WS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec DummyWriterContactStructs[] = {
    {"DummyWriterContactInfo", DummyWriterContactList,
     sizeof(struct _DummyWriterContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField DummyTimestepInfoList[] = {
    {"CheckString", "string", sizeof(char *),
     FMOffset(DummyPerTimestepInfo, CheckString)},
    {"CheckInt", "integer", sizeof(void *),
     FMOffset(DummyPerTimestepInfo, CheckInt)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec DummyTimestepInfoStructs[] = {
    {"DummyTimestepInfo", DummyTimestepInfoList,
     sizeof(struct _DummyPerTimestepInfo), NULL},
    {NULL, NULL, 0, NULL}};

static struct _CP_DP_Interface dummyDPInterface;

extern CP_DP_Interface LoadDummyDP()
{
    memset(&dummyDPInterface, 0, sizeof(dummyDPInterface));
    dummyDPInterface.ReaderContactFormats = DummyReaderContactStructs;
    dummyDPInterface.WriterContactFormats = DummyWriterContactStructs;
    dummyDPInterface.TimestepInfoFormats = NULL; // DummyTimestepInfoStructs;
    dummyDPInterface.initReader = DummyInitReader;
    dummyDPInterface.initWriter = DummyInitWriter;
    dummyDPInterface.initWriterPerReader = DummyInitWriterPerReader;
    dummyDPInterface.provideWriterDataToReader = DummyProvideWriterDataToReader;
    dummyDPInterface.readRemoteMemory = DummyReadRemoteMemory;
    dummyDPInterface.waitForCompletion = DummyWaitForCompletion;
    dummyDPInterface.provideTimestep = DummyProvideTimestep;
    dummyDPInterface.releaseTimestep = DummyReleaseTimestep;
    return &dummyDPInterface;
}
