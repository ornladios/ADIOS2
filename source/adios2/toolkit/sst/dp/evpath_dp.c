#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atl.h>
#include <evpath.h>

#include "sst_data.h"

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
    int Rank;

    /* writer info */
    int WriterCohortSize;
    CP_PeerCohort PeerCohort;
    struct _EvpathWriterContactInfo *WriterContactInfo;
} * Evpath_RS_Stream;

typedef struct _Evpath_WSR_Stream
{
    struct _Evpath_WS_Stream *WS_Stream;
    CP_PeerCohort PeerCohort;
    int ReaderCohortSize;
    struct _EvpathReaderContactInfo *ReaderContactInfo;
} * Evpath_WSR_Stream;

typedef struct _TimestepEntry
{
    long Timestep;
    struct _SstData *Data;
    struct _EvpathPerTimestepInfo *DP_TimestepInfo;
    struct _TimestepEntry *Next;

} * TimestepList;

typedef struct _Evpath_WS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;

    TimestepList Timesteps;
    CMFormat ReadReplyFormat;

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

static DP_RS_Stream EvpathInitReader(CP_Services Svcs, void *CP_Stream,
                                     void **ReaderContactInfoPtr)
{
    Evpath_RS_Stream Stream = malloc(sizeof(struct _Evpath_RS_Stream));
    EvpathReaderContactInfo Contact =
        malloc(sizeof(struct _EvpathReaderContactInfo));
    CManager cm = Svcs->getCManager(CP_Stream);
    char *EvpathContactString = malloc(64);
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

    MPI_Comm_rank(comm, &Stream->Rank);

    set_string_attr(ListenAttrs, attr_atom_from_string("CM_TRANSPORT"),
                    "sockets");

    CMlisten_specific(CM, ListenAttrs);
    attr_list ContactList = CMget_specific_contact_list(CM, ListenAttrs);

    EvpathContactString = attr_list_to_string(ContactList);

    /*
     * add a handler for read reply messages
     */
    Stream->ReadRequestFormat = CMregister_format(cm, EvpathReadRequestStructs);
    F = CMregister_format(cm, EvpathReadReplyStructs);
    CMregister_handler(F, EvpathReadReplyHandler, Svcs);

    Contact->ContactString = EvpathContactString;
    Contact->RS_Stream = Stream;

    *ReaderContactInfoPtr = Contact;

    return Stream;
}

static void EvpathReadRequestHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_Data,
                                     attr_list attrs)
{
    EvpathReadRequestMsg ReadRequestMsg = (EvpathReadRequestMsg)msg_v;
    Evpath_WSR_Stream WSR_Stream = ReadRequestMsg->WS_Stream;

    Evpath_WS_Stream WS_Stream = WSR_Stream->WS_Stream;
    TimestepList tmp = WS_Stream->Timesteps;
    CP_Services Svcs = (CP_Services)client_Data;

    Svcs->verbose(WS_Stream->CP_Stream, "Got a request to read remote memory "
                                        "from reader rank %d: timestep %d, "
                                        "offset %d, length %d\n",
                  ReadRequestMsg->RequestingRank, ReadRequestMsg->Timestep,
                  ReadRequestMsg->Offset, ReadRequestMsg->Length);
    while (tmp != NULL)
    {
        if (tmp->Timestep == ReadRequestMsg->Timestep)
        {
            struct _EvpathReadReplyMsg ReadReplyMsg;
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
            if (!WSR_Stream->ReaderContactInfo[ReadRequestMsg->RequestingRank]
                     .Conn)
            {
                attr_list List = attr_list_from_string(
                    WSR_Stream
                        ->ReaderContactInfo[ReadRequestMsg->RequestingRank]
                        .ContactString);
                CMConnection Conn = CMget_conn(cm, List);
                WSR_Stream->ReaderContactInfo[ReadRequestMsg->RequestingRank]
                    .Conn = Conn;
            }
            CMwrite(
                WSR_Stream->ReaderContactInfo[ReadRequestMsg->RequestingRank]
                    .Conn,
                WS_Stream->ReadReplyFormat, &ReadReplyMsg);

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
}

typedef struct _EvpathCompletionHandle
{
    int CMcondition;
    CManager cm;
    void *CPStream;
    void *Buffer;
    int Rank;
} * EvpathCompletionHandle;

static void EvpathReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                   void *client_Data, attr_list attrs)
{
    EvpathReadReplyMsg ReadReplyMsg = (EvpathReadReplyMsg)msg_v;
    Evpath_RS_Stream RS_Stream = ReadReplyMsg->RS_Stream;
    CP_Services Svcs = (CP_Services)client_Data;
    EvpathCompletionHandle Handle =
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
}

static DP_WS_Stream EvpathInitWriter(CP_Services Svcs, void *CP_Stream)
{
    Evpath_WS_Stream Stream = malloc(sizeof(struct _Evpath_WS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;

    memset(Stream, 0, sizeof(struct _Evpath_WS_Stream));

    MPI_Comm_rank(comm, &Stream->Rank);

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
     * register read reply message structure so we can send later
     */
    Stream->ReadReplyFormat = CMregister_format(cm, EvpathReadReplyStructs);

    return (void *)Stream;
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

    MPI_Comm_rank(comm, &Rank);
    sprintf(EvpathContactString, "Writer Rank %d, test contact", Rank);

    WSR_Stream->WS_Stream = WS_Stream; /* pointer to writer struct */
    WSR_Stream->PeerCohort = PeerCohort;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
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

    return WSR_Stream;
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

static void EvpathWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    EvpathCompletionHandle Handle = (EvpathCompletionHandle)Handle_v;
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

static void EvpathProvideTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                  struct _SstData *Data,
                                  struct _SstData *LocalMetadata, long Timestep,
                                  void **TimestepInfoPtr)
{
    Evpath_WS_Stream Stream = (Evpath_WS_Stream)Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));
    struct _EvpathPerTimestepInfo *Info =
        malloc(sizeof(struct _EvpathPerTimestepInfo));

    Info->CheckString = malloc(64);
    sprintf(Info->CheckString, "Evpath info for timestep %ld from rank %d",
            Timestep, Stream->Rank);
    Info->CheckInt = Stream->Rank * 1000 + Timestep;
    Entry->Data = Data;
    Entry->Timestep = Timestep;
    Entry->DP_TimestepInfo = Info;

    Entry->Next = Stream->Timesteps;
    Stream->Timesteps = Entry;
    *TimestepInfoPtr = Info;
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
    evpathDPInterface.provideTimestep = EvpathProvideTimestep;
    evpathDPInterface.releaseTimestep = EvpathReleaseTimestep;
    return &evpathDPInterface;
}
