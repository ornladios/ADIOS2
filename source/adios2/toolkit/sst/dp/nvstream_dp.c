#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atl.h>
#include <evpath.h>

#include "sst_data.h"

#include "adios2/toolkit/profiling/taustubs/taustubs.h"
#include "dp_interface.h"
#include "nvswrapper.h"

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
 *   This "nvstream" data plane uses control plane functionality to implement
 *   the ReadRemoteMemory functionality.  That is, it both the request to
 *   read memory and the response which carries the data are actually
 *   accomplished using the connections and message delivery facilities of
 *   the control plane, made available here via CP_Services.  A real data
 *   plane would replace one or both of these with RDMA functionality.
 */

typedef struct _Nvstream_RS_Stream
{
    CManager cm;
    void *CP_Stream;
    CMFormat ReadRequestFormat;
    pthread_mutex_t DataLock;
    int Rank;

    /* writer info */
    int WriterCohortSize;
    CP_PeerCohort PeerCohort;
    void **writer_nvs;
    struct _NvstreamWriterContactInfo *WriterContactInfo;
    struct _NvstreamCompletionHandle *PendingReadRequests;

    /* queued timestep info */
    struct _RSTimestepEntry *QueuedTimesteps;
} * Nvstream_RS_Stream;

typedef struct _Nvstream_WSR_Stream
{
    struct _Nvstream_WS_Stream *WS_Stream;
    CP_PeerCohort PeerCohort;
    int ReaderCohortSize;
    char *ReaderRequests;
    struct _NvstreamReaderContactInfo *ReaderContactInfo;
    struct _NvstreamWriterContactInfo
        *WriterContactInfo; /* included so we can free on destroy */
} * Nvstream_WSR_Stream;

typedef struct _TimestepEntry
{
    long Timestep;
    struct _SstData Data;
    struct _NvstreamPerTimestepInfo *DP_TimestepInfo;
    struct _TimestepEntry *Next;
} * TimestepList;

typedef struct _RSTimestepEntry
{
    long Timestep;
    int WriterRank;
    char *Data;
    long DataSize;
    long DataStart;
    struct _RSTimestepEntry *Next;
} * RSTimestepList;

typedef struct _Nvstream_WS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;
    void *nvs;

    TimestepList Timesteps;
    CMFormat ReadReplyFormat;

    int ReaderCount;
    Nvstream_WSR_Stream *Readers;
} * Nvstream_WS_Stream;

typedef struct _NvstreamReaderContactInfo
{
    char *ContactString;
    CMConnection Conn;
    void *RS_Stream;
} * NvstreamReaderContactInfo;

typedef struct _NvstreamWriterContactInfo
{
    char *ContactString;
    void *WS_Stream;
} * NvstreamWriterContactInfo;

typedef struct _NvstreamReadRequestMsg
{
    long Timestep;
    size_t Offset;
    size_t Length;
    void *WS_Stream;
    void *RS_Stream;
    int RequestingRank;
    int NotifyCondition;
} * NvstreamReadRequestMsg;

static FMField NvstreamReadRequestList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(NvstreamReadRequestMsg, Timestep)},
    {"Offset", "integer", sizeof(size_t),
     FMOffset(NvstreamReadRequestMsg, Offset)},
    {"Length", "integer", sizeof(size_t),
     FMOffset(NvstreamReadRequestMsg, Length)},
    {"WS_Stream", "integer", sizeof(void *),
     FMOffset(NvstreamReadRequestMsg, WS_Stream)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(NvstreamReadRequestMsg, RS_Stream)},
    {"RequestingRank", "integer", sizeof(int),
     FMOffset(NvstreamReadRequestMsg, RequestingRank)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(NvstreamReadRequestMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec NvstreamReadRequestStructs[] = {
    {"NvstreamReadRequest", NvstreamReadRequestList,
     sizeof(struct _NvstreamReadRequestMsg), NULL},
    {NULL, NULL, 0, NULL}};

typedef struct _NvstreamReadReplyMsg
{
    long Timestep;
    size_t DataLength;
    void *RS_Stream;
    char *Data;
    int NotifyCondition;
} * NvstreamReadReplyMsg;

static FMField NvstreamReadReplyList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(NvstreamReadReplyMsg, Timestep)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(NvstreamReadReplyMsg, RS_Stream)},
    {"DataLength", "integer", sizeof(size_t),
     FMOffset(NvstreamReadReplyMsg, DataLength)},
    {"Data", "char[DataLength]", sizeof(char),
     FMOffset(NvstreamReadReplyMsg, Data)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(NvstreamReadReplyMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec NvstreamReadReplyStructs[] = {
    {"NvstreamReadReply", NvstreamReadReplyList,
     sizeof(struct _NvstreamReadReplyMsg), NULL},
    {NULL, NULL, 0, NULL}};

static void NvstreamReadReplyHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_Data,
                                     attr_list attrs);

static DP_RS_Stream NvstreamInitReader(CP_Services Svcs, void *CP_Stream,
                                       void **ReaderContactInfoPtr,
                                       struct _SstParams *Params)
{
    Nvstream_RS_Stream Stream = malloc(sizeof(struct _Nvstream_RS_Stream));
    NvstreamReaderContactInfo Contact =
        malloc(sizeof(struct _NvstreamReaderContactInfo));
    CManager cm = Svcs->getCManager(CP_Stream);
    char *NvstreamContactString;
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;
    CManager CM = Svcs->getCManager(CP_Stream);
    attr_list ListenAttrs = create_attr_list();

    memset(Stream, 0, sizeof(*Stream));
    memset(Contact, 0, sizeof(*Contact));

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    pthread_mutex_init(&Stream->DataLock, NULL);

    SMPI_Comm_rank(comm, &Stream->Rank);

    set_string_attr(ListenAttrs, attr_atom_from_string("CM_TRANSPORT"),
                    "sockets");

    if (Params->DataInterface)
    {
        set_string_attr(ListenAttrs, attr_atom_from_string("IP_INTERFACE"),
                        strdup(Params->DataInterface));
    }
    else if (Params->NetworkInterface)
    {
        set_string_attr(ListenAttrs, attr_atom_from_string("IP_INTERFACE"),
                        strdup(Params->NetworkInterface));
    }

    CMlisten_specific(CM, ListenAttrs);
    attr_list ContactList = CMget_specific_contact_list(CM, ListenAttrs);

    NvstreamContactString = attr_list_to_string(ContactList);

    Contact->ContactString = NvstreamContactString;
    Contact->RS_Stream = Stream;

    *ReaderContactInfoPtr = Contact;

    return Stream;
}

static void NvstreamDestroyReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v)
{
    Nvstream_RS_Stream RS_Stream = (Nvstream_RS_Stream)RS_Stream_v;
    free(RS_Stream);
}

static void NvstreamReadRequestHandler(CManager cm, CMConnection conn,
                                       void *msg_v, void *client_Data,
                                       attr_list attrs)
{
    TAU_START_FUNC();
    NvstreamReadRequestMsg ReadRequestMsg = (NvstreamReadRequestMsg)msg_v;
    Nvstream_WSR_Stream WSR_Stream = ReadRequestMsg->WS_Stream;

    Nvstream_WS_Stream WS_Stream = WSR_Stream->WS_Stream;
    TimestepList tmp = WS_Stream->Timesteps;
    CP_Services Svcs = (CP_Services)client_Data;
    int RequestingRank = ReadRequestMsg->RequestingRank;

    Svcs->verbose(WS_Stream->CP_Stream,
                  "Got a request to read remote memory "
                  "from reader rank %d: timestep %d, "
                  "offset %d, length %d\n",
                  RequestingRank, ReadRequestMsg->Timestep,
                  ReadRequestMsg->Offset, ReadRequestMsg->Length);
    while (tmp != NULL)
    {
        if (tmp->Timestep == ReadRequestMsg->Timestep)
        {
            struct _NvstreamReadReplyMsg ReadReplyMsg;
            /* memset avoids uninit byte warnings from valgrind */
            memset(&ReadReplyMsg, 0, sizeof(ReadReplyMsg));
            ReadReplyMsg.Timestep = ReadRequestMsg->Timestep;
            ReadReplyMsg.DataLength = ReadRequestMsg->Length;
            ReadReplyMsg.Data = tmp->Data.block + ReadRequestMsg->Offset;
            ReadReplyMsg.RS_Stream = ReadRequestMsg->RS_Stream;
            ReadReplyMsg.NotifyCondition = ReadRequestMsg->NotifyCondition;
            Svcs->verbose(
                WS_Stream->CP_Stream,
                "Sending a reply to reader rank %d for remote memory read\n",
                RequestingRank);
            if (!WSR_Stream->ReaderContactInfo[RequestingRank].Conn)
            {
                attr_list List = attr_list_from_string(
                    WSR_Stream->ReaderContactInfo[RequestingRank]
                        .ContactString);
                CMConnection Conn = CMget_conn(cm, List);
                free_attr_list(List);
                WSR_Stream->ReaderContactInfo[RequestingRank].Conn = Conn;
            }
            CMwrite(WSR_Stream->ReaderContactInfo[RequestingRank].Conn,
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
    fprintf(stderr, "Writer rank %d - Failed to read Timestep %ld, not found\n",
            WSR_Stream->WS_Stream->Rank, ReadRequestMsg->Timestep);
    /*
     * in the interest of not failing a writer on a reader failure, don't
     * assert(0) here.  Probably this sort of error should close the link to
     * a reader though.
     */
    TAU_STOP_FUNC();
}

typedef struct _NvstreamCompletionHandle
{
    int CMcondition;
    CManager cm;
    void *CPStream;
    void *DPStream;
    void *Buffer;
    int Failed;
    int Rank;
    struct _NvstreamCompletionHandle *Next;
} * NvstreamCompletionHandle;

static void NvstreamReadReplyHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_Data,
                                     attr_list attrs)
{
    TAU_START_FUNC();
    NvstreamReadReplyMsg ReadReplyMsg = (NvstreamReadReplyMsg)msg_v;
    Nvstream_RS_Stream RS_Stream = ReadReplyMsg->RS_Stream;
    CP_Services Svcs = (CP_Services)client_Data;
    NvstreamCompletionHandle Handle = NULL;

    if (CMCondition_has_signaled(cm, ReadReplyMsg->NotifyCondition))
    {
        Svcs->verbose(RS_Stream->CP_Stream, "Got a reply to remote memory "
                                            "read, but the condition is "
                                            "already signalled, returning\n");
        TAU_STOP_FUNC();
        return;
    }
    Handle = CMCondition_get_client_data(cm, ReadReplyMsg->NotifyCondition);

    if (!Handle)
    {
        Svcs->verbose(
            RS_Stream->CP_Stream,
            "Got a reply to remote memory read, but condition not found\n");
        TAU_STOP_FUNC();
        return;
    }
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

static DP_WS_Stream NvstreamInitWriter(CP_Services Svcs, void *CP_Stream,
                                       struct _SstParams *Params)
{
    Nvstream_WS_Stream Stream = malloc(sizeof(struct _Nvstream_WS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    memset(Stream, 0, sizeof(struct _Nvstream_WS_Stream));

    SMPI_Comm_rank(comm, &Stream->Rank);

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    Stream->nvs = nvs_create_store();

    Svcs->verbose(Stream->CP_Stream,
                  "Initializing NVSTREAM writer, create store returned %p\n",
                  Stream->nvs);

    return (void *)Stream;
}

static void NvstreamDestroyWriter(CP_Services Svcs, DP_WS_Stream WS_Stream_v)
{
    Nvstream_WS_Stream WS_Stream = (Nvstream_WS_Stream)WS_Stream_v;
    nvs_finalize_(WS_Stream->nvs);
    for (int i = 0; i < WS_Stream->ReaderCount; i++)
    {
        if (WS_Stream->Readers[i])
        {
            free(WS_Stream->Readers[i]->WriterContactInfo->ContactString);
            free(WS_Stream->Readers[i]->WriterContactInfo);
            free(WS_Stream->Readers[i]->ReaderContactInfo->ContactString);
            if (WS_Stream->Readers[i]->ReaderContactInfo->Conn)
                CMConnection_close(
                    WS_Stream->Readers[i]->ReaderContactInfo->Conn);
            free(WS_Stream->Readers[i]->ReaderContactInfo);
            free(WS_Stream->Readers[i]);
        }
    }
    free(WS_Stream->Readers);
    free(WS_Stream);
}

static DP_WSR_Stream NvstreamInitWriterPerReader(CP_Services Svcs,
                                                 DP_WS_Stream WS_Stream_v,
                                                 int readerCohortSize,
                                                 CP_PeerCohort PeerCohort,
                                                 void **providedReaderInfo_v,
                                                 void **WriterContactInfoPtr)
{
    Nvstream_WS_Stream WS_Stream = (Nvstream_WS_Stream)WS_Stream_v;
    Nvstream_WSR_Stream WSR_Stream = malloc(sizeof(*WSR_Stream));
    NvstreamWriterContactInfo ContactInfo;
    MPI_Comm comm = Svcs->getMPIComm(WS_Stream->CP_Stream);
    int Rank;
    NvstreamReaderContactInfo *providedReaderInfo =
        (NvstreamReaderContactInfo *)providedReaderInfo_v;

    SMPI_Comm_rank(comm, &Rank);

    WSR_Stream->WS_Stream = WS_Stream; /* pointer to writer struct */
    WSR_Stream->PeerCohort = PeerCohort;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    WSR_Stream->ReaderCohortSize = readerCohortSize;
    WSR_Stream->ReaderContactInfo =
        malloc(sizeof(struct _NvstreamReaderContactInfo) * readerCohortSize);
    for (int i = 0; i < readerCohortSize; i++)
    {
        WSR_Stream->ReaderContactInfo[i].ContactString = NULL;
        WSR_Stream->ReaderContactInfo[i].Conn = NULL;
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

    ContactInfo = malloc(sizeof(struct _NvstreamWriterContactInfo));
    memset(ContactInfo, 0, sizeof(struct _NvstreamWriterContactInfo));
    ContactInfo->ContactString = strdup(nvs_get_store_name(WS_Stream->nvs));
    ContactInfo->WS_Stream = WSR_Stream;
    *WriterContactInfoPtr = ContactInfo;
    WSR_Stream->WriterContactInfo = ContactInfo;

    return WSR_Stream;
}

static void NvstreamDestroyWriterPerReader(CP_Services Svcs,
                                           DP_WSR_Stream WSR_Stream_v)
{
    Nvstream_WSR_Stream WSR_Stream = (Nvstream_WSR_Stream)WSR_Stream_v;
    free(WSR_Stream);
}

static void NvstreamProvideWriterDataToReader(CP_Services Svcs,
                                              DP_RS_Stream RS_Stream_v,
                                              int writerCohortSize,
                                              CP_PeerCohort PeerCohort,
                                              void **providedWriterInfo_v)
{
    Nvstream_RS_Stream RS_Stream = (Nvstream_RS_Stream)RS_Stream_v;
    NvstreamWriterContactInfo *providedWriterInfo =
        (NvstreamWriterContactInfo *)providedWriterInfo_v;

    RS_Stream->PeerCohort = PeerCohort;
    RS_Stream->WriterCohortSize = writerCohortSize;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    RS_Stream->WriterContactInfo =
        malloc(sizeof(struct _NvstreamWriterContactInfo) * writerCohortSize);
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
    RS_Stream->writer_nvs = malloc(sizeof(void *) * writerCohortSize);
    for (int i = 0; i < writerCohortSize; i++)
    {
        RS_Stream->writer_nvs[i] =
            nvs_open_store(RS_Stream->WriterContactInfo[i].ContactString);
    }
}

static void AddRequestToList(CP_Services Svcs, Nvstream_RS_Stream Stream,
                             NvstreamCompletionHandle Handle)
{
    Handle->Next = Stream->PendingReadRequests;
    Stream->PendingReadRequests = Handle;
}

static void RemoveRequestFromList(CP_Services Svcs, Nvstream_RS_Stream Stream,
                                  NvstreamCompletionHandle Handle)
{
    NvstreamCompletionHandle Tmp = Stream->PendingReadRequests;

    if (Stream->PendingReadRequests == Handle)
    {
        Stream->PendingReadRequests = Handle->Next;
        return;
    }

    while (Tmp != NULL && Tmp->Next != Handle)
    {
        Tmp = Tmp->Next;
    }

    if (Tmp == NULL)
        return;

    // Tmp->Next must be the handle to remove
    Tmp->Next = Tmp->Next->Next;
}

static void FailRequestsToRank(CP_Services Svcs, CManager cm,
                               Nvstream_RS_Stream Stream, int FailedRank)
{
    NvstreamCompletionHandle Tmp = Stream->PendingReadRequests;
    Svcs->verbose(Stream->CP_Stream,
                  "Fail pending requests to writer rank %d\n", FailedRank);
    while (Tmp != NULL)
    {
        if (Tmp->Rank == FailedRank)
        {
            Tmp->Failed = 1;
            Svcs->verbose(Tmp->CPStream,
                          "Found a pending remote memory read "
                          "to failed writer rank %d, marking as "
                          "failed and signalling condition %d\n",
                          Tmp->Rank, Tmp->CMcondition);
            CMCondition_signal(cm, Tmp->CMcondition);
            Svcs->verbose(Tmp->CPStream, "Did the signal of condition %d\n",
                          Tmp->Rank, Tmp->CMcondition);
        }
        Tmp = Tmp->Next;
    }
    Svcs->verbose(Stream->CP_Stream,
                  "Done Failing requests to writer rank %d\n", FailedRank);
}

typedef struct _NvstreamPerTimestepInfo
{
    char *CheckString;
    int CheckInt;
} * NvstreamPerTimestepInfo;

static void *NvstreamReadRemoteMemory(CP_Services Svcs, DP_RS_Stream Stream_v,
                                      int Rank, long Timestep, size_t Offset,
                                      size_t Length, void *Buffer,
                                      void *DP_TimestepInfo)
{
    Nvstream_RS_Stream Stream = (Nvstream_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    NvstreamCompletionHandle ret =
        malloc(sizeof(struct _NvstreamCompletionHandle));
    NvstreamPerTimestepInfo TimestepInfo =
        (NvstreamPerTimestepInfo)DP_TimestepInfo;
    struct _NvstreamReadRequestMsg ReadRequestMsg;

    static long LastRequestedTimestep = -1;

    LastRequestedTimestep = Timestep;

    ret->CPStream = Stream->CP_Stream;
    ret->DPStream = Stream;
    ret->Failed = 0;
    ret->cm = cm;
    ret->Buffer = Buffer;
    ret->Rank = Rank;
    ret->CMcondition = -1;

    Svcs->verbose(Stream->CP_Stream,
                  "Adios requesting to read remote memory for Timestep %d "
                  "from Rank %d, WSR_Stream = %p, nvs = %p\n",
                  Timestep, Rank, Stream->WriterContactInfo[Rank].WS_Stream,
                  Stream->writer_nvs[Rank]);

    char *StringName = malloc(20);
    void *NVBlock;
    char *BaseAddr;
    sprintf(StringName, "Timestep_%ld", Timestep);
    BaseAddr = nvs_get_with_malloc(Stream->writer_nvs[Rank], StringName, 1);

    if (BaseAddr)
    {
        memcpy(Buffer, BaseAddr + Offset, Length);
    }
    else
    {
        fprintf(stderr, "remote memory read failed\n");
    }
    return ret;
}

static int NvstreamWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    NvstreamCompletionHandle Handle = (NvstreamCompletionHandle)Handle_v;
    int Ret = 1;
    Svcs->verbose(
        Handle->CPStream,
        "Waiting for completion of memory read to rank %d, condition %d\n",
        Handle->Rank, Handle->CMcondition);
    /*
     * Wait for the CM condition to be signalled.  If it has been already,
     * this returns immediately.  Copying the incoming data to the waiting
     * buffer has been done by the reply handler.
     */
    if (Handle->CMcondition != -1)
        CMCondition_wait(Handle->cm, Handle->CMcondition);
    if (Handle->Failed)
    {
        Svcs->verbose(Handle->CPStream,
                      "Remote memory read to rank %d with "
                      "condition %d has FAILED because of "
                      "writer failure\n",
                      Handle->Rank, Handle->CMcondition);
        Ret = 0;
    }
    else
    {
        Svcs->verbose(
            Handle->CPStream,
            "Remote memory read to rank %d with condition %d has completed\n",
            Handle->Rank, Handle->CMcondition);
    }
    RemoveRequestFromList(Svcs, Handle->DPStream, Handle);
    free(Handle);
    return Ret;
}

static void NvstreamNotifyConnFailure(CP_Services Svcs, DP_RS_Stream Stream_v,
                                      int FailedPeerRank)
{
    Nvstream_RS_Stream Stream = (Nvstream_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    Svcs->verbose(Stream->CP_Stream,
                  "received notification that writer peer "
                  "%d has failed, failing any pending "
                  "requests\n",
                  FailedPeerRank);
    FailRequestsToRank(Svcs, cm, Stream, FailedPeerRank);
}

static void NvstreamProvideTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                    struct _SstData *Data,
                                    struct _SstData *LocalMetadata,
                                    long Timestep, void **TimestepInfoPtr)
{
    Nvstream_WS_Stream Stream = (Nvstream_WS_Stream)Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));
    struct _NvstreamPerTimestepInfo *Info = NULL;
    //        malloc(sizeof(struct _NvstreamPerTimestepInfo));

    //  This section exercised the CP's ability to distribute DP per timestep
    //  info. Commenting out as needed for Nvstream for now
    //    Info->CheckString = malloc(64);
    //    sprintf(Info->CheckString, "Nvstream info for timestep %ld from rank
    //    %d",
    //            Timestep, Stream->Rank);
    //    Info->CheckInt = Stream->Rank * 1000 + Timestep;
    //    *TimestepInfoPtr = Info;
    memset(Entry, 0, sizeof(*Entry));
    Entry->DP_TimestepInfo = NULL;
    Entry->Data = *Data;
    Entry->Timestep = Timestep;

    Entry->Next = Stream->Timesteps;
    Stream->Timesteps = Entry;
    *TimestepInfoPtr = NULL;

    char *StringName = malloc(20);
    void *NVBlock;
    sprintf(StringName, "Timestep_%ld", Timestep);
    NVBlock = nvs_alloc(Stream->nvs, &Data->DataSize, StringName);
    memcpy(NVBlock, Data->block, Data->DataSize);
    nvs_snapshot_(Stream->nvs, &Stream->Rank);
    fprintf(stderr, "Did alloc, memcpy and snapshot with name %s\n",
            StringName);
}

static void NvstreamReleaseTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                    long Timestep)
{
    Nvstream_WS_Stream Stream = (Nvstream_WS_Stream)Stream_v;
    TimestepList List = Stream->Timesteps;

    Svcs->verbose(Stream->CP_Stream, "Releasing timestep %ld\n", Timestep);
    if (Stream->Timesteps->Timestep == Timestep)
    {
        Stream->Timesteps = List->Next;
        if (List->DP_TimestepInfo && List->DP_TimestepInfo->CheckString)
            free(List->DP_TimestepInfo->CheckString);
        if (List->DP_TimestepInfo)
            free(List->DP_TimestepInfo);
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
                if (List->DP_TimestepInfo && List->DP_TimestepInfo->CheckString)
                    free(List->DP_TimestepInfo->CheckString);
                if (List->DP_TimestepInfo)
                    free(List->DP_TimestepInfo);
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

static FMField NvstreamReaderContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(NvstreamReaderContactInfo, ContactString)},
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(NvstreamReaderContactInfo, RS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec NvstreamReaderContactStructs[] = {
    {"NvstreamReaderContactInfo", NvstreamReaderContactList,
     sizeof(struct _NvstreamReaderContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField NvstreamWriterContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(NvstreamWriterContactInfo, ContactString)},
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(NvstreamWriterContactInfo, WS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec NvstreamWriterContactStructs[] = {
    {"NvstreamWriterContactInfo", NvstreamWriterContactList,
     sizeof(struct _NvstreamWriterContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField NvstreamTimestepInfoList[] = {
    {"CheckString", "string", sizeof(char *),
     FMOffset(NvstreamPerTimestepInfo, CheckString)},
    {"CheckInt", "integer", sizeof(void *),
     FMOffset(NvstreamPerTimestepInfo, CheckInt)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec NvstreamTimestepInfoStructs[] = {
    {"NvstreamTimestepInfo", NvstreamTimestepInfoList,
     sizeof(struct _NvstreamPerTimestepInfo), NULL},
    {NULL, NULL, 0, NULL}};

static struct _CP_DP_Interface nvstreamDPInterface;

static int NvstreamGetPriority(CP_Services Svcs, void *CP_Stream,
                               struct _SstParams *Params)
{
    /* The nvstream DP priority 10 */
    return 10;
}

extern CP_DP_Interface LoadNvstreamDP()
{
    memset(&nvstreamDPInterface, 0, sizeof(nvstreamDPInterface));
    nvstreamDPInterface.ReaderContactFormats = NvstreamReaderContactStructs;
    nvstreamDPInterface.WriterContactFormats = NvstreamWriterContactStructs;
    nvstreamDPInterface.TimestepInfoFormats =
        NULL; // NvstreamTimestepInfoStructs;
    nvstreamDPInterface.initReader = NvstreamInitReader;
    nvstreamDPInterface.initWriter = NvstreamInitWriter;
    nvstreamDPInterface.initWriterPerReader = NvstreamInitWriterPerReader;
    nvstreamDPInterface.provideWriterDataToReader =
        NvstreamProvideWriterDataToReader;
    nvstreamDPInterface.readRemoteMemory = NvstreamReadRemoteMemory;
    nvstreamDPInterface.waitForCompletion = NvstreamWaitForCompletion;
    nvstreamDPInterface.notifyConnFailure = NvstreamNotifyConnFailure;
    nvstreamDPInterface.provideTimestep = NvstreamProvideTimestep;
    nvstreamDPInterface.releaseTimestep = NvstreamReleaseTimestep;
    nvstreamDPInterface.readerRegisterTimestep = NULL;
    nvstreamDPInterface.readerReleaseTimestep = NULL;
    nvstreamDPInterface.readPatternLocked = NULL;
    nvstreamDPInterface.destroyReader = NvstreamDestroyReader;
    nvstreamDPInterface.destroyWriter = NvstreamDestroyWriter;
    nvstreamDPInterface.destroyWriterPerReader = NvstreamDestroyWriterPerReader;
    nvstreamDPInterface.getPriority = NvstreamGetPriority;
    nvstreamDPInterface.unGetPriority = NULL;
    return &nvstreamDPInterface;
}
