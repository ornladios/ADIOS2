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
 *   This "evpath" data plane uses control plane functionality to implement
 *   the ReadRemoteMemory functionality.  That is, it both the request to
 *   read memory and the response which carries the data are actually
 *   accomplished using the connections and message delivery facilities of
 *   the control plane, made available here via CP_Services.  A real data
 *   plane would replace one or both of these with RDMA functionality.
 */

typedef struct _Evpath_RS_Stream
{
    CManager cm;
    void *CP_Stream;
    CMFormat ReadRequestFormat;
    pthread_mutex_t DataLock;
    int Rank;

    /* writer info */
    int WriterCohortSize;
    CP_PeerCohort PeerCohort;
    struct _EvpathWriterContactInfo *WriterContactInfo;
    struct _EvpathCompletionHandle *PendingReadRequests;

    /* queued timestep info */
    struct _RSTimestepEntry *QueuedTimesteps;
} * Evpath_RS_Stream;

typedef struct _Evpath_WSR_Stream
{
    struct _Evpath_WS_Stream *WS_Stream;
    CP_PeerCohort PeerCohort;
    int ReaderCohortSize;
    int ReadPatternLockTimestep;
    char *ReaderRequests;
    struct _EvpathReaderContactInfo *ReaderContactInfo;
    struct _EvpathWriterContactInfo
        *WriterContactInfo; /* included so we can free on destroy */
} * Evpath_WSR_Stream;

typedef struct _TimestepEntry
{
    long Timestep;
    struct _SstData Data;
    struct _EvpathPerTimestepInfo *DP_TimestepInfo;
    struct _ReaderRequestTrackRec *ReaderRequests;
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

typedef struct _ReaderRequestTrackRec
{
    Evpath_WSR_Stream Reader;
    char *RequestList;
    struct _ReaderRequestTrackRec *Next;
} * ReaderRequestTrackPtr;

typedef struct _Evpath_WS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;

    TimestepList Timesteps;
    CMFormat ReadReplyFormat;
    CMFormat PreloadFormat;

    int ReaderCount;
    Evpath_WSR_Stream *Readers;
} * Evpath_WS_Stream;

typedef struct _EvpathReaderContactInfo
{
    char *ContactString;
    CMConnection Conn;
    void *RS_Stream;
} * EvpathReaderContactInfo;

typedef struct _EvpathWriterContactInfo
{
    char *ContactString;
    void *WS_Stream;
} * EvpathWriterContactInfo;

typedef struct _EvpathReadRequestMsg
{
    long Timestep;
    size_t Offset;
    size_t Length;
    void *WS_Stream;
    void *RS_Stream;
    int RequestingRank;
    int NotifyCondition;
} * EvpathReadRequestMsg;

static FMField EvpathReadRequestList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(EvpathReadRequestMsg, Timestep)},
    {"Offset", "integer", sizeof(size_t),
     FMOffset(EvpathReadRequestMsg, Offset)},
    {"Length", "integer", sizeof(size_t),
     FMOffset(EvpathReadRequestMsg, Length)},
    {"WS_Stream", "integer", sizeof(void *),
     FMOffset(EvpathReadRequestMsg, WS_Stream)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(EvpathReadRequestMsg, RS_Stream)},
    {"RequestingRank", "integer", sizeof(int),
     FMOffset(EvpathReadRequestMsg, RequestingRank)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(EvpathReadRequestMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec EvpathReadRequestStructs[] = {
    {"EvpathReadRequest", EvpathReadRequestList,
     sizeof(struct _EvpathReadRequestMsg), NULL},
    {NULL, NULL, 0, NULL}};

typedef struct _EvpathReadReplyMsg
{
    long Timestep;
    size_t DataLength;
    void *RS_Stream;
    char *Data;
    int NotifyCondition;
} * EvpathReadReplyMsg;

static FMField EvpathReadReplyList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(EvpathReadReplyMsg, Timestep)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(EvpathReadReplyMsg, RS_Stream)},
    {"DataLength", "integer", sizeof(size_t),
     FMOffset(EvpathReadReplyMsg, DataLength)},
    {"Data", "char[DataLength]", sizeof(char),
     FMOffset(EvpathReadReplyMsg, Data)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(EvpathReadReplyMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec EvpathReadReplyStructs[] = {
    {"EvpathReadReply", EvpathReadReplyList, sizeof(struct _EvpathReadReplyMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static void EvpathReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                   void *client_Data, attr_list attrs);

typedef struct _EvpathPreloadMsg
{
    long Timestep;
    size_t DataLength;
    int WriterRank;
    void *RS_Stream;
    char *Data;
} * EvpathPreloadMsg;

static FMField EvpathPreloadList[] = {
    {"Timestep", "integer", sizeof(long), FMOffset(EvpathPreloadMsg, Timestep)},
    {"DataLength", "integer", sizeof(size_t),
     FMOffset(EvpathPreloadMsg, DataLength)},
    {"WriterRank", "integer", sizeof(size_t),
     FMOffset(EvpathPreloadMsg, WriterRank)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(EvpathPreloadMsg, RS_Stream)},
    {"Data", "char[DataLength]", sizeof(char),
     FMOffset(EvpathPreloadMsg, Data)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec EvpathPreloadStructs[] = {
    {"EvpathPreload", EvpathPreloadList, sizeof(struct _EvpathPreloadMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static void EvpathPreloadHandler(CManager cm, CMConnection conn, void *msg_v,
                                 void *client_Data, attr_list attrs);
static void DiscardPriorPreloaded(CP_Services Svcs, Evpath_RS_Stream RS_Stream,
                                  long Timestep);
static void SendPreloadMsgs(CP_Services Svcs, Evpath_WSR_Stream WSR_Stream,
                            TimestepList TS);
static void SendSpeculativePreloadMsgs(CP_Services Svcs,
                                       Evpath_WSR_Stream WSR_Stream,
                                       TimestepList TS);

static DP_RS_Stream EvpathInitReader(CP_Services Svcs, void *CP_Stream,
                                     void **ReaderContactInfoPtr,
                                     struct _SstParams *Params,
                                     attr_list WriterContact)
{
    Evpath_RS_Stream Stream = malloc(sizeof(struct _Evpath_RS_Stream));
    EvpathReaderContactInfo Contact =
        malloc(sizeof(struct _EvpathReaderContactInfo));
    CManager cm = Svcs->getCManager(CP_Stream);
    char *EvpathContactString;
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

    EvpathContactString = attr_list_to_string(ContactList);

    /*
     * add a handler for read reply messages
     */
    Stream->ReadRequestFormat = CMregister_format(cm, EvpathReadRequestStructs);
    F = CMregister_format(cm, EvpathReadReplyStructs);
    CMregister_handler(F, EvpathReadReplyHandler, Svcs);

    /*
     * add a handler for timestep preload messages
     */
    F = CMregister_format(cm, EvpathPreloadStructs);
    CMregister_handler(F, EvpathPreloadHandler, Svcs);

    Contact->ContactString = EvpathContactString;
    Contact->RS_Stream = Stream;

    *ReaderContactInfoPtr = Contact;

    return Stream;
}

static void EvpathDestroyReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v)
{
    Evpath_RS_Stream RS_Stream = (Evpath_RS_Stream)RS_Stream_v;
    DiscardPriorPreloaded(Svcs, RS_Stream, -1);
    free(RS_Stream);
}

static void MarkReadRequest(TimestepList TS, DP_WSR_Stream Reader,
                            int RequestingRank)
{
    ReaderRequestTrackPtr ReqList = TS->ReaderRequests;
    while (ReqList != NULL)
    {
        if (ReqList->Reader == Reader)
        {
            ReqList->RequestList[RequestingRank] = 1;
        }
        ReqList = ReqList->Next;
    }
}

static void EvpathReadRequestHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_Data,
                                     attr_list attrs)
{
    TAU_START_FUNC();
    EvpathReadRequestMsg ReadRequestMsg = (EvpathReadRequestMsg)msg_v;
    Evpath_WSR_Stream WSR_Stream = ReadRequestMsg->WS_Stream;

    Evpath_WS_Stream WS_Stream = WSR_Stream->WS_Stream;
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
            struct _EvpathReadReplyMsg ReadReplyMsg;
            /* memset avoids uninit byte warnings from valgrind */
            MarkReadRequest(tmp, WSR_Stream, RequestingRank);
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
                if (!Conn)
                {
                    Svcs->verbose(
                        WS_Stream->CP_Stream,
                        "Failed to connect to reader rank %d for response to "
                        "remote read, assume failure, no response sent\n",
                        RequestingRank);
                    TAU_STOP_FUNC();
                    return;
                }
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
    fprintf(stderr, "\n\n\n\n");
    fprintf(stderr,
            "Writer rank %d - Failed to read Timestep %ld, not found.  This is "
            "an internal inconsistency\n",
            WSR_Stream->WS_Stream->Rank, ReadRequestMsg->Timestep);
    fprintf(stderr,
            "Writer rank %d - Request came from rank %d, please report this "
            "error!\n",
            WSR_Stream->WS_Stream->Rank, RequestingRank);
    fprintf(stderr, "\n\n\n\n");

    /*
     * in the interest of not failing a writer on a reader failure, don't
     * assert(0) here.  Probably this sort of error should close the link to
     * a reader though.
     */
    TAU_STOP_FUNC();
}

typedef struct _EvpathCompletionHandle
{
    int CMcondition;
    CManager cm;
    void *CPStream;
    void *DPStream;
    void *Buffer;
    int Failed;
    int Rank;
    struct _EvpathCompletionHandle *Next;
} * EvpathCompletionHandle;

static void EvpathReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                   void *client_Data, attr_list attrs)
{
    TAU_START_FUNC();
    EvpathReadReplyMsg ReadReplyMsg = (EvpathReadReplyMsg)msg_v;
    Evpath_RS_Stream RS_Stream = ReadReplyMsg->RS_Stream;
    CP_Services Svcs = (CP_Services)client_Data;
    EvpathCompletionHandle Handle = NULL;

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

static int HandleRequestWithPreloaded(CP_Services Svcs,
                                      Evpath_RS_Stream RS_Stream, int Rank,
                                      long Timestep, size_t Offset,
                                      size_t Length, void *Buffer)
{
    RSTimestepList Entry = NULL;
    pthread_mutex_lock(&RS_Stream->DataLock);
    Entry = RS_Stream->QueuedTimesteps;
    while (Entry &&
           ((Entry->WriterRank != Rank) || (Entry->Timestep != Timestep)))
    {
        Entry = Entry->Next;
    }
    pthread_mutex_unlock(&RS_Stream->DataLock);
    if (!Entry)
    {
        return 0;
    }
    Svcs->verbose(RS_Stream->CP_Stream,
                  "Satisfying remote memory read with preload from writer rank "
                  "%d for timestep %ld\n",
                  Rank, Timestep);
    memcpy(Buffer, Entry->Data + Offset, Length);
    return 1;
}

static void DiscardPriorPreloaded(CP_Services Svcs, Evpath_RS_Stream RS_Stream,
                                  long Timestep)
{
    RSTimestepList Entry, Last = NULL;
    pthread_mutex_lock(&RS_Stream->DataLock);
    Entry = RS_Stream->QueuedTimesteps;

    while (Entry)
    {
        RSTimestepList Next = Entry->Next;
        if ((Entry->Timestep < Timestep) || (Timestep == -1))
        {
            RSTimestepList ItemToFree = Entry;
            CManager cm = Svcs->getCManager(RS_Stream->CP_Stream);
            if (Last)
            {
                Last->Next = Entry->Next;
            }
            else
            {
                RS_Stream->QueuedTimesteps = Entry->Next;
            }
            /* free item */
            CMreturn_buffer(cm, ItemToFree->Data);

            free(ItemToFree);
        }
        else
        {
            Last = Entry;
        }
        Entry = Next;
    }
    pthread_mutex_unlock(&RS_Stream->DataLock);
}

static void EvpathPreloadHandler(CManager cm, CMConnection conn, void *msg_v,
                                 void *client_Data, attr_list attrs)
{
    EvpathPreloadMsg PreloadMsg = (EvpathPreloadMsg)msg_v;
    Evpath_RS_Stream RS_Stream = PreloadMsg->RS_Stream;
    CP_Services Svcs = (CP_Services)client_Data;
    EvpathCompletionHandle Handle = NULL;
    RSTimestepList Entry = calloc(1, sizeof(*Entry));

    Svcs->verbose(
        RS_Stream->CP_Stream,
        "Got a preload message from writer rank %d for timestep %ld\n",
        PreloadMsg->WriterRank, PreloadMsg->Timestep);

    /* arrange for this message data to stay around */
    CMtake_buffer(cm, msg_v);

    Entry->Timestep = PreloadMsg->Timestep;
    Entry->WriterRank = PreloadMsg->WriterRank;
    Entry->Data = PreloadMsg->Data;
    Entry->DataSize = PreloadMsg->DataLength;
    Entry->DataStart = 0;

    pthread_mutex_lock(&RS_Stream->DataLock);
    Entry->Next = RS_Stream->QueuedTimesteps;
    RS_Stream->QueuedTimesteps = Entry;
    pthread_mutex_unlock(&RS_Stream->DataLock);
    return;
}

static DP_WS_Stream EvpathInitWriter(CP_Services Svcs, void *CP_Stream,
                                     struct _SstParams *Params,
                                     attr_list DPAttrs)
{
    Evpath_WS_Stream Stream = malloc(sizeof(struct _Evpath_WS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    memset(Stream, 0, sizeof(struct _Evpath_WS_Stream));

    SMPI_Comm_rank(comm, &Stream->Rank);

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    /*
     * add a handler for read request messages
     */
    F = CMregister_format(cm, EvpathReadRequestStructs);
    CMregister_handler(F, EvpathReadRequestHandler, Svcs);

    /*
     * Register for sending preload messages
     */
    Stream->PreloadFormat = CMregister_format(cm, EvpathPreloadStructs);
    /*
     * register read reply message structure so we can send later
     */
    Stream->ReadReplyFormat = CMregister_format(cm, EvpathReadReplyStructs);

    //   We'd set an attribute here if we needed to communicate it to possible
    //   readers
    //    set_int_attr(DPAttrs, attr_atom_from_string("EVPATH_DP_Attr"),
    //    0xdeadbeef);

    return (void *)Stream;
}

static void EvpathDestroyWriter(CP_Services Svcs, DP_WS_Stream WS_Stream_v)
{
    Evpath_WS_Stream WS_Stream = (Evpath_WS_Stream)WS_Stream_v;
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

static DP_WSR_Stream EvpathInitWriterPerReader(CP_Services Svcs,
                                               DP_WS_Stream WS_Stream_v,
                                               int readerCohortSize,
                                               CP_PeerCohort PeerCohort,
                                               void **providedReaderInfo_v,
                                               void **WriterContactInfoPtr)
{
    Evpath_WS_Stream WS_Stream = (Evpath_WS_Stream)WS_Stream_v;
    Evpath_WSR_Stream WSR_Stream = malloc(sizeof(*WSR_Stream));
    EvpathWriterContactInfo ContactInfo;
    MPI_Comm comm = Svcs->getMPIComm(WS_Stream->CP_Stream);
    int Rank;
    char *EvpathContactString = malloc(64);
    EvpathReaderContactInfo *providedReaderInfo =
        (EvpathReaderContactInfo *)providedReaderInfo_v;

    SMPI_Comm_rank(comm, &Rank);
    sprintf(EvpathContactString, "Writer Rank %d, test contact", Rank);

    WSR_Stream->WS_Stream = WS_Stream; /* pointer to writer struct */
    WSR_Stream->PeerCohort = PeerCohort;
    WSR_Stream->ReadPatternLockTimestep = -1;
    WSR_Stream->ReaderRequests = NULL;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    WSR_Stream->ReaderCohortSize = readerCohortSize;
    WSR_Stream->ReaderContactInfo =
        malloc(sizeof(struct _EvpathReaderContactInfo) * readerCohortSize);
    for (int i = 0; i < readerCohortSize; i++)
    {
        WSR_Stream->ReaderContactInfo[i].ContactString =
            strdup(providedReaderInfo[i]->ContactString);
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

    ContactInfo = malloc(sizeof(struct _EvpathWriterContactInfo));
    memset(ContactInfo, 0, sizeof(struct _EvpathWriterContactInfo));
    ContactInfo->ContactString = EvpathContactString;
    ContactInfo->WS_Stream = WSR_Stream;
    *WriterContactInfoPtr = ContactInfo;
    WSR_Stream->WriterContactInfo = ContactInfo;

    return WSR_Stream;
}

static void EvpathDestroyWriterPerReader(CP_Services Svcs,
                                         DP_WSR_Stream WSR_Stream_v)
{
    Evpath_WSR_Stream WSR_Stream = (Evpath_WSR_Stream)WSR_Stream_v;
    free(WSR_Stream);
}

static void EvpathProvideWriterDataToReader(CP_Services Svcs,
                                            DP_RS_Stream RS_Stream_v,
                                            int writerCohortSize,
                                            CP_PeerCohort PeerCohort,
                                            void **providedWriterInfo_v)
{
    Evpath_RS_Stream RS_Stream = (Evpath_RS_Stream)RS_Stream_v;
    EvpathWriterContactInfo *providedWriterInfo =
        (EvpathWriterContactInfo *)providedWriterInfo_v;

    RS_Stream->PeerCohort = PeerCohort;
    RS_Stream->WriterCohortSize = writerCohortSize;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    RS_Stream->WriterContactInfo =
        malloc(sizeof(struct _EvpathWriterContactInfo) * writerCohortSize);
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

static void AddRequestToList(CP_Services Svcs, Evpath_RS_Stream Stream,
                             EvpathCompletionHandle Handle)
{
    Handle->Next = Stream->PendingReadRequests;
    Stream->PendingReadRequests = Handle;
}

static void RemoveRequestFromList(CP_Services Svcs, Evpath_RS_Stream Stream,
                                  EvpathCompletionHandle Handle)
{
    EvpathCompletionHandle Tmp = Stream->PendingReadRequests;

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
                               Evpath_RS_Stream Stream, int FailedRank)
{
    EvpathCompletionHandle Tmp = Stream->PendingReadRequests;
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

typedef struct _EvpathPerTimestepInfo
{
    char *CheckString;
    int CheckInt;
} * EvpathPerTimestepInfo;

static void *EvpathReadRemoteMemory(CP_Services Svcs, DP_RS_Stream Stream_v,
                                    int Rank, long Timestep, size_t Offset,
                                    size_t Length, void *Buffer,
                                    void *DP_TimestepInfo)
{
    Evpath_RS_Stream Stream = (Evpath_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    EvpathCompletionHandle ret = malloc(sizeof(struct _EvpathCompletionHandle));
    EvpathPerTimestepInfo TimestepInfo = (EvpathPerTimestepInfo)DP_TimestepInfo;
    struct _EvpathReadRequestMsg ReadRequestMsg;

    int HadPreload;
    static long LastRequestedTimestep = -1;

    if ((LastRequestedTimestep != -1) && (LastRequestedTimestep != Timestep))
    {
        DiscardPriorPreloaded(Svcs, Stream, Timestep);
    }
    LastRequestedTimestep = Timestep;
    HadPreload = HandleRequestWithPreloaded(Svcs, Stream, Rank, Timestep,
                                            Offset, Length, Buffer);
    ret->CPStream = Stream->CP_Stream;
    ret->DPStream = Stream;
    ret->Failed = 0;
    ret->cm = cm;
    ret->Buffer = Buffer;
    ret->Rank = Rank;

    if (HadPreload)
    {
        /* cool, we already had the data.  Setup a dummy return handle */
        ret->CMcondition = -1;
        return ret;
    }

    ret->CMcondition = CMCondition_get(cm, NULL);

    /*
     * set the completion handle as client Data on the condition so that
     * handler has access to it.
     */
    AddRequestToList(Svcs, Stream, ret);
    CMCondition_set_client_data(cm, ret->CMcondition, ret);

    Svcs->verbose(Stream->CP_Stream,
                  "Adios requesting to read remote memory for Timestep %d "
                  "from Rank %d, WSR_Stream = %p, DP_TimestepInfo %p\n",
                  Timestep, Rank, Stream->WriterContactInfo[Rank].WS_Stream,
                  DP_TimestepInfo);

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
    if (!Svcs->sendToPeer(Stream->CP_Stream, Stream->PeerCohort, Rank,
                          Stream->ReadRequestFormat, &ReadRequestMsg))
    {
        ret->Failed = 1;
        CMCondition_signal(cm, ret->CMcondition);
    }

    return ret;
}

static int EvpathWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    EvpathCompletionHandle Handle = (EvpathCompletionHandle)Handle_v;
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

static void EvpathNotifyConnFailure(CP_Services Svcs, DP_RS_Stream Stream_v,
                                    int FailedPeerRank)
{
    Evpath_RS_Stream Stream = (Evpath_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    Svcs->verbose(Stream->CP_Stream,
                  "received notification that writer peer "
                  "%d has failed, failing any pending "
                  "requests\n",
                  FailedPeerRank);
    FailRequestsToRank(Svcs, cm, Stream, FailedPeerRank);
}

static void EvpathWSReaderRegisterTimestep(CP_Services Svcs,
                                           DP_WSR_Stream WSRStream_v,
                                           long Timestep,
                                           SstPreloadModeType PreloadMode)
{
    Evpath_WSR_Stream WSR_Stream = (Evpath_WSR_Stream)WSRStream_v;
    Evpath_WS_Stream WS_Stream =
        WSR_Stream->WS_Stream; /* pointer to writer struct */
    TimestepList Entry = WS_Stream->Timesteps;

    while (Entry)
    {
        if (Entry->Timestep == Timestep)
        {
            break;
        }
        Entry = Entry->Next;
    }
    if (!Entry)
    {
        fprintf(
            stderr,
            "Didn't find timestep in per reader register, shouldn't happen\n");
        return;
    }
    /* go ahead and record read patterns, just in case we need them */
    ReaderRequestTrackPtr ReqTrk = calloc(1, sizeof(*ReqTrk));
    ReqTrk->Reader = WSR_Stream;
    ReqTrk->RequestList = calloc(1, WSR_Stream->ReaderCohortSize);
    ReqTrk->Next = Entry->ReaderRequests;
    Entry->ReaderRequests = ReqTrk;

    Svcs->verbose(WS_Stream->CP_Stream,
                  "Per reader registration for timestep %ld, preload mode %d\n",
                  Timestep, PreloadMode);
    if (PreloadMode == SstPreloadLearned)
    {
        if (WSR_Stream->ReaderRequests)
        {
            Svcs->verbose(
                WS_Stream->CP_Stream,
                "Sending Learned Preload messages, reader %p, timestep %ld\n",
                WSR_Stream, Timestep);
            SendPreloadMsgs(Svcs, WSR_Stream, Entry);
        }
    }
    else if (PreloadMode == SstPreloadSpeculative)
    {
        Svcs->verbose(
            WS_Stream->CP_Stream,
            "Sending Speculative Preload messages, reader %p, timestep %ld\n",
            WSR_Stream, Timestep);
        SendSpeculativePreloadMsgs(Svcs, WSR_Stream, Entry);
    }
}

static void EvpathRSTimestepArrived(CP_Services Svcs, DP_RS_Stream RS_Stream_v,
                                    long Timestep,
                                    SstPreloadModeType PreloadMode)
{
    Evpath_RS_Stream RS_Stream = (Evpath_RS_Stream)RS_Stream_v;
    Svcs->verbose(
        RS_Stream->CP_Stream,
        "EVPATH registering reader arrival of TS %ld metadata, preload mode "
        "%d\n",
        Timestep, PreloadMode);
}

static void SendPreloadMsgs(CP_Services Svcs, Evpath_WSR_Stream WSR_Stream,
                            TimestepList TS)
{
    Evpath_WS_Stream WS_Stream =
        WSR_Stream->WS_Stream; /* pointer to writer struct */
    struct _EvpathPreloadMsg PreloadMsg;
    memset(&PreloadMsg, 0, sizeof(PreloadMsg));
    PreloadMsg.Timestep = TS->Timestep;
    PreloadMsg.DataLength = TS->Data.DataSize;
    PreloadMsg.Data = TS->Data.block;
    PreloadMsg.WriterRank = WS_Stream->Rank;

    for (int i = 0; i < WSR_Stream->ReaderCohortSize; i++)
    {
        if (WSR_Stream->ReaderRequests[i])
        {
            PreloadMsg.RS_Stream = WSR_Stream->ReaderContactInfo[i].RS_Stream;
            CMwrite(WSR_Stream->ReaderContactInfo[i].Conn,
                    WS_Stream->PreloadFormat, &PreloadMsg);
        }
    }
}

static void SendSpeculativePreloadMsgs(CP_Services Svcs,
                                       Evpath_WSR_Stream WSR_Stream,
                                       TimestepList TS)
{
    Evpath_WS_Stream WS_Stream =
        WSR_Stream->WS_Stream; /* pointer to writer struct */
    CManager cm = Svcs->getCManager(WS_Stream->CP_Stream);
    struct _EvpathPreloadMsg PreloadMsg;
    memset(&PreloadMsg, 0, sizeof(PreloadMsg));
    PreloadMsg.Timestep = TS->Timestep;
    PreloadMsg.DataLength = TS->Data.DataSize;
    PreloadMsg.Data = TS->Data.block;
    PreloadMsg.WriterRank = WS_Stream->Rank;

    for (int i = 0; i < WSR_Stream->ReaderCohortSize; i++)
    {
        if (!WSR_Stream->ReaderContactInfo[i].Conn)
        {
            attr_list List = attr_list_from_string(
                WSR_Stream->ReaderContactInfo[i].ContactString);
            CMConnection Conn = CMget_conn(cm, List);
            free_attr_list(List);
            if (!Conn)
            {
                Svcs->verbose(
                    WS_Stream->CP_Stream,
                    "Failed to connect to reader rank %d for response to "
                    "remote read, assume failure, no response sent\n",
                    i);
                return;
            }
            WSR_Stream->ReaderContactInfo[i].Conn = Conn;
        }
        PreloadMsg.RS_Stream = WSR_Stream->ReaderContactInfo[i].RS_Stream;
        CMwrite(WSR_Stream->ReaderContactInfo[i].Conn, WS_Stream->PreloadFormat,
                &PreloadMsg);
    }
}

static void EvpathReaderReleaseTimestep(CP_Services Svcs,
                                        DP_WSR_Stream Stream_v, long Timestep)
{
    Evpath_WSR_Stream WSR_Stream = (Evpath_WSR_Stream)Stream_v;
    Evpath_WS_Stream WS_Stream =
        WSR_Stream->WS_Stream; /* pointer to writer struct */
    TimestepList tmp = WS_Stream->Timesteps;

    if ((!WSR_Stream->ReaderRequests) &&
        (Timestep >= WSR_Stream->ReadPatternLockTimestep))
    {
        /* save the pattern */
        while (tmp != NULL)
        {
            if (tmp->Timestep == Timestep)
            {
                ReaderRequestTrackPtr ReqList = tmp->ReaderRequests;
                while (ReqList != NULL)
                {
                    if (ReqList->Reader == WSR_Stream)
                    {
                        WSR_Stream->ReaderRequests = ReqList->RequestList;
                        /* so it doesn't get free'd */
                        ReqList->RequestList = NULL;
                    }
                    ReqList = ReqList->Next;
                }
            }
            tmp = tmp->Next;
        }
        if (WSR_Stream->ReaderRequests)
        {
            /* send out any already queued timesteps */
            tmp = WS_Stream->Timesteps;
            while (tmp != NULL)
            {
                if (tmp->Timestep <= WSR_Stream->ReadPatternLockTimestep)
                {
                    SendPreloadMsgs(Svcs, WSR_Stream, tmp);
                }
                tmp = tmp->Next;
            }
        }
    }
}

static void EvpathWSRReadPatternLocked(CP_Services Svcs,
                                       DP_WSR_Stream WSRStream_v,
                                       long EffectiveTimestep)
{
    Evpath_WSR_Stream WSR_Stream = (Evpath_WSR_Stream)WSRStream_v;
    Evpath_WS_Stream WS_Stream =
        WSR_Stream->WS_Stream; /* pointer to writer struct */

    Svcs->verbose(WS_Stream->CP_Stream,
                  "Locking the read pattern at Timestep %ld\n",
                  EffectiveTimestep);
    WSR_Stream->ReadPatternLockTimestep = EffectiveTimestep;
}

static void EvpathProvideTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                  struct _SstData *Data,
                                  struct _SstData *LocalMetadata, long Timestep,
                                  void **TimestepInfoPtr)
{
    Evpath_WS_Stream Stream = (Evpath_WS_Stream)Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));
    struct _EvpathPerTimestepInfo *Info = NULL;
    //        malloc(sizeof(struct _EvpathPerTimestepInfo));

    //  This section exercised the CP's ability to distribute DP per timestep
    //  info. Commenting out as needed for EVPath for now
    //    Info->CheckString = malloc(64);
    //    sprintf(Info->CheckString, "Evpath info for timestep %ld from rank
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
}

static void EvpathReleaseTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                  long Timestep)
{
    Evpath_WS_Stream Stream = (Evpath_WS_Stream)Stream_v;
    TimestepList List = Stream->Timesteps;

    Svcs->verbose(Stream->CP_Stream, "Releasing timestep %ld\n", Timestep);
    if (Stream->Timesteps->Timestep == Timestep)
    {
        Stream->Timesteps = List->Next;
        if (List->DP_TimestepInfo && List->DP_TimestepInfo->CheckString)
            free(List->DP_TimestepInfo->CheckString);
        if (List->DP_TimestepInfo)
            free(List->DP_TimestepInfo);
        if (List->ReaderRequests)
        {
            ReaderRequestTrackPtr tmp = List->ReaderRequests;
            while (tmp)
            {
                ReaderRequestTrackPtr Next = tmp->Next;
                if (tmp->RequestList)
                    free(tmp->RequestList);
                free(tmp);
                tmp = Next;
            }
        }

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
                if (List->ReaderRequests)
                {
                    ReaderRequestTrackPtr tmp = List->ReaderRequests;
                    while (tmp)
                    {
                        ReaderRequestTrackPtr Next = tmp->Next;
                        if (tmp->RequestList)
                            free(tmp->RequestList);
                        free(tmp);
                        tmp = Next;
                    }
                }

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

static FMField EvpathReaderContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(EvpathReaderContactInfo, ContactString)},
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(EvpathReaderContactInfo, RS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec EvpathReaderContactStructs[] = {
    {"EvpathReaderContactInfo", EvpathReaderContactList,
     sizeof(struct _EvpathReaderContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField EvpathWriterContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(EvpathWriterContactInfo, ContactString)},
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(EvpathWriterContactInfo, WS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec EvpathWriterContactStructs[] = {
    {"EvpathWriterContactInfo", EvpathWriterContactList,
     sizeof(struct _EvpathWriterContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField EvpathTimestepInfoList[] = {
    {"CheckString", "string", sizeof(char *),
     FMOffset(EvpathPerTimestepInfo, CheckString)},
    {"CheckInt", "integer", sizeof(void *),
     FMOffset(EvpathPerTimestepInfo, CheckInt)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec EvpathTimestepInfoStructs[] = {
    {"EvpathTimestepInfo", EvpathTimestepInfoList,
     sizeof(struct _EvpathPerTimestepInfo), NULL},
    {NULL, NULL, 0, NULL}};

static struct _CP_DP_Interface evpathDPInterface;

static int EvpathGetPriority(CP_Services Svcs, void *CP_Stream,
                             struct _SstParams *Params)
{
    // Define any unique attributes here
    (void)attr_atom_from_string("EVPATH_DP_Attr");

    /* The evpath DP should be a lower priority than any RDMA dp, so return 1 */
    return 1;
}

extern CP_DP_Interface LoadEVpathDP()
{
    memset(&evpathDPInterface, 0, sizeof(evpathDPInterface));
    evpathDPInterface.ReaderContactFormats = EvpathReaderContactStructs;
    evpathDPInterface.WriterContactFormats = EvpathWriterContactStructs;
    evpathDPInterface.TimestepInfoFormats = NULL; // EvpathTimestepInfoStructs;
    evpathDPInterface.initReader = EvpathInitReader;
    evpathDPInterface.initWriter = EvpathInitWriter;
    evpathDPInterface.initWriterPerReader = EvpathInitWriterPerReader;
    evpathDPInterface.provideWriterDataToReader =
        EvpathProvideWriterDataToReader;
    evpathDPInterface.readRemoteMemory = EvpathReadRemoteMemory;
    evpathDPInterface.waitForCompletion = EvpathWaitForCompletion;
    evpathDPInterface.notifyConnFailure = EvpathNotifyConnFailure;
    evpathDPInterface.provideTimestep = EvpathProvideTimestep;
    evpathDPInterface.releaseTimestep = EvpathReleaseTimestep;
    evpathDPInterface.readerRegisterTimestep = EvpathWSReaderRegisterTimestep;
    evpathDPInterface.readerReleaseTimestep = EvpathReaderReleaseTimestep;
    evpathDPInterface.WSRreadPatternLocked = EvpathWSRReadPatternLocked;
    evpathDPInterface.RSreadPatternLocked = NULL;
    evpathDPInterface.timestepArrived = EvpathRSTimestepArrived;
    evpathDPInterface.destroyReader = EvpathDestroyReader;
    evpathDPInterface.destroyWriter = EvpathDestroyWriter;
    evpathDPInterface.destroyWriterPerReader = EvpathDestroyWriterPerReader;
    evpathDPInterface.getPriority = EvpathGetPriority;
    evpathDPInterface.unGetPriority = NULL;
    return &evpathDPInterface;
}
