#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atl.h>
#include <evpath.h>
#include <mpi.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>

#include "sst_data.h"

#include "dp_interface.h"

#define DP_AV_DEF_SIZE 512

pthread_mutex_t fabric_mutex = PTHREAD_MUTEX_INITIALIZER;

struct fabric_state
{
    struct fi_context *ctx;
    // struct fi_context *lctx;
    struct fi_info *info;
    // struct fi_info *linfo;
    int local_mr_req;
    size_t addr_len;
    size_t msg_prefix_size;
    // size_t lmsg_prefix_size;
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_ep *signal;
    struct fid_cq *cq_signal;
    struct fid_av *av;
    pthread_t listener;
};

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
 *   This "Rdma" data plane uses control plane functionality to implement
 *   the ReadRemoteMemory functionality.  That is, it both the request to
 *   read memory and the response which carries the data are actually
 *   accomplished using the connections and message delivery facilities of
 *   the control plane, made available here via CP_Services.  A real data
 *   plane would replace one or both of these with RDMA functionality.
 */

static void init_fabric(struct fabric_state *fabric)
{
    struct fi_info *hints, *info;
    struct fi_av_attr av_attr = {0};
    struct fi_cq_attr cq_attr = {0};
    char *ifname;

    hints = fi_allocinfo();
    hints->caps = FI_MSG | FI_SEND | FI_RECV | FI_REMOTE_READ |
                  FI_REMOTE_WRITE | FI_RMA | FI_READ | FI_WRITE;
    hints->mode = FI_CONTEXT | FI_LOCAL_MR | FI_CONTEXT2 | FI_MSG_PREFIX |
                  FI_ASYNC_IOV | FI_RX_CQ_DATA;
    hints->domain_attr->mr_mode = FI_MR_BASIC;

    ifname = getenv("FABRIC_IFACE");

    fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, hints, &info);
    fi_freeinfo(hints);

    if (!info)
    {
        fabric->info = NULL;
        return;
    }
    if (info->mode & FI_CONTEXT2)
    {
        fabric->ctx = calloc(2, sizeof(*fabric->ctx));
    }
    else if (info->mode & FI_CONTEXT)
    {
        fabric->ctx = calloc(1, sizeof(*fabric->ctx));
    }
    else
    {
        fabric->ctx = NULL;
    }

    if (info->mode & FI_LOCAL_MR)
    {
        fabric->local_mr_req = 1;
    }
    else
    {
        fabric->local_mr_req = 0;
    }

    if (info->mode & FI_MSG_PREFIX)
    {
        fabric->msg_prefix_size = info->ep_attr->msg_prefix_size;
    }
    else
    {
        fabric->msg_prefix_size = 0;
    }

    while (info->next)
    {
        if (ifname && strcmp(ifname, info->domain_attr->name) == 0)
        {
            break;
        }
        info = info->next;
    }

    fabric->info = fi_dupinfo(info);
    fabric->addr_len = info->src_addrlen;

    info->domain_attr->mr_mode = FI_MR_BASIC;
    fi_fabric(info->fabric_attr, &fabric->fabric, fabric->ctx);
    fi_domain(fabric->fabric, info, &fabric->domain, fabric->ctx);
    info->ep_attr->type = FI_EP_RDM;
    fi_endpoint(fabric->domain, info, &fabric->signal, fabric->ctx);

    av_attr.type = FI_AV_MAP;
    av_attr.count = DP_AV_DEF_SIZE;
    av_attr.ep_per_node = 0;
    fi_av_open(fabric->domain, &av_attr, &fabric->av, fabric->ctx);
    fi_ep_bind(fabric->signal, &fabric->av->fid, 0);

    cq_attr.size = 0;
    cq_attr.format = FI_CQ_FORMAT_DATA;
    cq_attr.wait_obj = FI_WAIT_UNSPEC;
    cq_attr.wait_cond = FI_CQ_COND_NONE;
    fi_cq_open(fabric->domain, &cq_attr, &fabric->cq_signal, fabric->ctx);
    fi_ep_bind(fabric->signal, &fabric->cq_signal->fid, FI_TRANSMIT | FI_RECV);

    fi_enable(fabric->signal);
}

typedef struct fabric_state *FabricState;

typedef struct _Rdma_RS_Stream
{
    CManager cm;
    void *CP_Stream;
    CMFormat ReadRequestFormat;
    int Rank;
    FabricState Fabric;

    /* writer info */
    int WriterCohortSize;
    CP_PeerCohort PeerCohort;
    struct _RdmaWriterContactInfo *WriterContactInfo;
    fi_addr_t *WriterAddr;
} * Rdma_RS_Stream;

typedef struct _Rdma_WSR_Stream
{
    struct _Rdma_WS_Stream *WS_Stream;
    CP_PeerCohort PeerCohort;
    int ReaderCohortSize;
    struct _RdmaReaderContactInfo *ReaderContactInfo;
} * Rdma_WSR_Stream;

typedef struct _TimestepEntry
{
    long Timestep;
    struct _SstData *Data;
    struct _RdmaPerTimestepInfo *DP_TimestepInfo;
    struct _TimestepEntry *Next;
    struct fid_mr *mr;
    uint64_t Key;
} * TimestepList;

typedef struct _Rdma_WS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;
    FabricState Fabric;

    TimestepList Timesteps;
    CMFormat ReadReplyFormat;

    int ReaderCount;
    Rdma_WSR_Stream *Readers;
} * Rdma_WS_Stream;

typedef struct _RdmaReaderContactInfo
{
    char *ContactString;
    void *RS_Stream;
} * RdmaReaderContactInfo;

typedef struct _RdmaWriterContactInfo
{
    char *ContactString;
    void *WS_Stream;
    size_t Length;
    void *Address;
} * RdmaWriterContactInfo;

typedef struct _RdmaReadRequestMsg
{
    long Timestep;
    size_t Offset;
    size_t Length;
    void *WS_Stream;
    void *RS_Stream;
    int RequestingRank;
    int NotifyCondition;
} * RdmaReadRequestMsg;

static FMField RdmaReadRequestList[] = {
    {"Timestep", "integer", sizeof(long),
     FMOffset(RdmaReadRequestMsg, Timestep)},
    {"Offset", "integer", sizeof(size_t), FMOffset(RdmaReadRequestMsg, Offset)},
    {"Length", "integer", sizeof(size_t), FMOffset(RdmaReadRequestMsg, Length)},
    {"WS_Stream", "integer", sizeof(void *),
     FMOffset(RdmaReadRequestMsg, WS_Stream)},
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(RdmaReadRequestMsg, RS_Stream)},
    {"RequestingRank", "integer", sizeof(int),
     FMOffset(RdmaReadRequestMsg, RequestingRank)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(RdmaReadRequestMsg, NotifyCondition)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaReadRequestStructs[] = {
    {"RdmaReadRequest", RdmaReadRequestList, sizeof(struct _RdmaReadRequestMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

typedef struct _RdmaReadReplyMsg
{
    void *RS_Stream;
    int NotifyCondition;
    uint64_t Key;
    void *Addr;
} * RdmaReadReplyMsg;

static FMField RdmaReadReplyList[] = {
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(RdmaReadReplyMsg, RS_Stream)},
    {"NotifyCondition", "integer", sizeof(int),
     FMOffset(RdmaReadReplyMsg, NotifyCondition)},
    {"Key", "integer", sizeof(uint64_t), FMOffset(RdmaReadReplyMsg, Key)},
    {"Addr", "integer", sizeof(void *), FMOffset(RdmaReadReplyMsg, Addr)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaReadReplyStructs[] = {
    {"RdmaReadReply", RdmaReadReplyList, sizeof(struct _RdmaReadReplyMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static void RdmaReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                 void *client_Data, attr_list attrs);

static DP_RS_Stream RdmaInitReader(CP_Services Svcs, void *CP_Stream,
                                   void **ReaderContactInfoPtr)
{
    Rdma_RS_Stream Stream = malloc(sizeof(struct _Rdma_RS_Stream));
    RdmaReaderContactInfo Contact =
        malloc(sizeof(struct _RdmaReaderContactInfo));
    CManager cm = Svcs->getCManager(CP_Stream);
    char *RdmaContactString = malloc(64);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;
    FabricState Fabric;

    memset(Stream, 0, sizeof(*Stream));
    memset(Contact, 0, sizeof(*Contact));

    Stream->Fabric = calloc(1, sizeof(struct fabric_state));
    init_fabric(Stream->Fabric);
    Fabric = Stream->Fabric;
    if (!Fabric->info)
    {
        Svcs->verbose(CP_Stream, "Could not find a valid transport fabric.\n");
        free(Stream);
        free(Contact);
        free(RdmaContactString);
        *ReaderContactInfoPtr = NULL;
        return (NULL);
    }

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    MPI_Comm_rank(comm, &Stream->Rank);

    sprintf(RdmaContactString, "Reader Rank %d, test contact", Stream->Rank);

    /*
     * add a handler for read reply messages
     */
    Stream->ReadRequestFormat = CMregister_format(cm, RdmaReadRequestStructs);
    F = CMregister_format(cm, RdmaReadReplyStructs);
    CMregister_handler(F, RdmaReadReplyHandler, Svcs);

    Contact->ContactString = RdmaContactString;
    Contact->RS_Stream = Stream;

    *ReaderContactInfoPtr = Contact;

    return Stream;
}

static void RdmaReadRequestHandler(CManager cm, CMConnection conn, void *msg_v,
                                   void *client_Data, attr_list attrs)
{
    RdmaReadRequestMsg ReadRequestMsg = (RdmaReadRequestMsg)msg_v;
    Rdma_WSR_Stream WSR_Stream = ReadRequestMsg->WS_Stream;

    Rdma_WS_Stream WS_Stream = WSR_Stream->WS_Stream;
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
            struct _RdmaReadReplyMsg ReadReplyMsg;
            /* memset avoids uninit byte warnings from valgrind */
            memset(&ReadReplyMsg, 0, sizeof(ReadReplyMsg));
            //            ReadReplyMsg.Timestep = ReadRequestMsg->Timestep;
            //            ReadReplyMsg.DataLength = ReadRequestMsg->Length;
            //            ReadReplyMsg.Data = tmp->Data->block +
            //            ReadRequestMsg->Offset;
            ReadReplyMsg.RS_Stream = ReadRequestMsg->RS_Stream;
            ReadReplyMsg.NotifyCondition = ReadRequestMsg->NotifyCondition;
            ReadReplyMsg.Key = tmp->Key;
            ReadReplyMsg.Addr = tmp->Data->block + ReadRequestMsg->Offset;
            Svcs->verbose(
                WS_Stream->CP_Stream,
                "Sending a reply to reader rank %d for remote memory read\n",
                ReadRequestMsg->RequestingRank);
            Svcs->sendToPeer(WS_Stream->CP_Stream, WSR_Stream->PeerCohort,
                             ReadRequestMsg->RequestingRank,
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

typedef struct _RdmaCompletionHandle
{
    int CMcondition;
    CManager cm;
    void *CPStream;
    void *Buffer;
    size_t Length;
    int Rank;
} * RdmaCompletionHandle;

static void RdmaReadReplyHandler(CManager cm, CMConnection conn, void *msg_v,
                                 void *client_Data, attr_list attrs)
{
    RdmaReadReplyMsg ReadReplyMsg = (RdmaReadReplyMsg)msg_v;
    Rdma_RS_Stream RS_Stream = ReadReplyMsg->RS_Stream;
    FabricState Fabric = RS_Stream->Fabric;
    CP_Services Svcs = (CP_Services)client_Data;
    RdmaCompletionHandle Handle =
        CMCondition_get_client_data(cm, ReadReplyMsg->NotifyCondition);
    void *LocalDesc = NULL;
    fi_addr_t SrcAddress;
    struct fid_mr *LocalMR;
    struct fi_cq_data_entry CQEntry = {0};
    ssize_t rc;

    Svcs->verbose(
        RS_Stream->CP_Stream,
        "Got a reply to remote memory read from rank %d, condition is %d\n",
        Handle->Rank, ReadReplyMsg->NotifyCondition);

    SrcAddress = RS_Stream->WriterAddr[Handle->Rank];

    pthread_mutex_lock(&fabric_mutex);
    if (Fabric->local_mr_req)
    {
        // register dest buffer
        fi_mr_reg(Fabric->domain, Handle->Buffer, Handle->Length, FI_READ, 0, 0,
                  0, &LocalMR, Fabric->ctx);
        LocalDesc = fi_mr_desc(LocalMR);
    }

    do
    {
        rc = fi_read(Fabric->signal, Handle->Buffer, Handle->Length, LocalDesc,
                     SrcAddress, (uint64_t)ReadReplyMsg->Addr,
                     ReadReplyMsg->Key, Fabric->ctx);
    } while (rc == -EAGAIN);

    if (rc != 0)
    {
        Svcs->verbose(RS_Stream->CP_Stream, "fi_readmsg failed with code %d.\n",
                      rc);
    }
    else
    {
        fi_cq_sread(Fabric->cq_signal, (void *)(&CQEntry), 1, NULL, -1);
    }

    if (Fabric->local_mr_req)
    {
        fi_close((struct fid *)LocalMR);
    }
    pthread_mutex_unlock(&fabric_mutex);

    /*
     * Signal the condition to wake the reader if they are waiting.
     */
    CMCondition_signal(cm, ReadReplyMsg->NotifyCondition);
}

static DP_WS_Stream RdmaInitWriter(CP_Services Svcs, void *CP_Stream)
{
    Rdma_WS_Stream Stream = malloc(sizeof(struct _Rdma_WS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    CMFormat F;
    FabricState Fabric;

    memset(Stream, 0, sizeof(struct _Rdma_WS_Stream));

    Stream->Fabric = calloc(1, sizeof(struct fabric_state));
    init_fabric(Stream->Fabric);
    Fabric = Stream->Fabric;
    if (!Fabric->info)
    {
        Svcs->verbose(CP_Stream, "Could not find a valid transport fabric.\n");
        free(Stream);
        return (NULL);
    }

    MPI_Comm_rank(comm, &Stream->Rank);

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    /*
     * add a handler for read request messages
     */
    F = CMregister_format(cm, RdmaReadRequestStructs);
    CMregister_handler(F, RdmaReadRequestHandler, Svcs);

    /*
     * register read reply message structure so we can send later
     */
    Stream->ReadReplyFormat = CMregister_format(cm, RdmaReadReplyStructs);

    return (void *)Stream;
}

static DP_WSR_Stream RdmaInitWriterPerReader(CP_Services Svcs,
                                             DP_WS_Stream WS_Stream_v,
                                             int readerCohortSize,
                                             CP_PeerCohort PeerCohort,
                                             void **providedReaderInfo_v,
                                             void **WriterContactInfoPtr)
{
    Rdma_WS_Stream WS_Stream = (Rdma_WS_Stream)WS_Stream_v;
    Rdma_WSR_Stream WSR_Stream = malloc(sizeof(*WSR_Stream));
    FabricState Fabric = WS_Stream->Fabric;
    RdmaWriterContactInfo ContactInfo;
    MPI_Comm comm = Svcs->getMPIComm(WS_Stream->CP_Stream);
    int Rank;
    char *RdmaContactString = malloc(64);
    RdmaReaderContactInfo *providedReaderInfo =
        (RdmaReaderContactInfo *)providedReaderInfo_v;

    MPI_Comm_rank(comm, &Rank);
    sprintf(RdmaContactString, "Writer Rank %d, test contact", Rank);

    WSR_Stream->WS_Stream = WS_Stream; /* pointer to writer struct */
    WSR_Stream->PeerCohort = PeerCohort;

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    WSR_Stream->ReaderContactInfo =
        malloc(sizeof(struct _RdmaReaderContactInfo) * readerCohortSize);
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

    ContactInfo = malloc(sizeof(struct _RdmaWriterContactInfo));
    memset(ContactInfo, 0, sizeof(struct _RdmaWriterContactInfo));
    ContactInfo->ContactString = RdmaContactString;
    ContactInfo->WS_Stream = WSR_Stream;

    ContactInfo->Length = Fabric->info->src_addrlen;
    ContactInfo->Address = malloc(ContactInfo->Length);
    fi_getname((fid_t)Fabric->signal, ContactInfo->Address,
               &ContactInfo->Length);

    *WriterContactInfoPtr = ContactInfo;

    return WSR_Stream;
}

static void RdmaProvideWriterDataToReader(CP_Services Svcs,
                                          DP_RS_Stream RS_Stream_v,
                                          int writerCohortSize,
                                          CP_PeerCohort PeerCohort,
                                          void **providedWriterInfo_v)
{
    Rdma_RS_Stream RS_Stream = (Rdma_RS_Stream)RS_Stream_v;
    FabricState Fabric = RS_Stream->Fabric;
    RdmaWriterContactInfo *providedWriterInfo =
        (RdmaWriterContactInfo *)providedWriterInfo_v;

    RS_Stream->PeerCohort = PeerCohort;
    RS_Stream->WriterCohortSize = writerCohortSize;
    RS_Stream->WriterAddr =
        calloc(writerCohortSize, sizeof(*RS_Stream->WriterAddr));

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    RS_Stream->WriterContactInfo =
        malloc(sizeof(struct _RdmaWriterContactInfo) * writerCohortSize);
    for (int i = 0; i < writerCohortSize; i++)
    {
        RS_Stream->WriterContactInfo[i].ContactString =
            strdup(providedWriterInfo[i]->ContactString);
        RS_Stream->WriterContactInfo[i].WS_Stream =
            providedWriterInfo[i]->WS_Stream;
        fi_av_insert(Fabric->av, providedWriterInfo[i]->Address, 1,
                     &RS_Stream->WriterAddr[i], 0, NULL);
        Svcs->verbose(
            RS_Stream->CP_Stream,
            "Received contact info \"%s\", WS_stream %p for WSR Rank %d\n",
            RS_Stream->WriterContactInfo[i].ContactString,
            RS_Stream->WriterContactInfo[i].WS_Stream, i);
    }
}

typedef struct _RdmaPerTimestepInfo
{
    char *CheckString;
    int CheckInt;
    pthread_mutex_t CheckLock;
} * RdmaPerTimestepInfo;

static void *RdmaReadRemoteMemory(CP_Services Svcs, DP_RS_Stream Stream_v,
                                  int Rank, long Timestep, size_t Offset,
                                  size_t Length, void *Buffer,
                                  void *DP_TimestepInfo)
{
    Rdma_RS_Stream Stream = (Rdma_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    RdmaCompletionHandle ret = malloc(sizeof(struct _RdmaCompletionHandle));
    struct _RdmaReadRequestMsg ReadRequestMsg;

    ret->CMcondition = CMCondition_get(cm, NULL);
    ret->CPStream = Stream->CP_Stream;
    ret->cm = cm;
    ret->Buffer = Buffer;
    ret->Rank = Rank;
    ret->Length = Length;
    /*
     * set the completion handle as client Data on the condition so that
     * handler has access to it.
     */
    CMCondition_set_client_data(cm, ret->CMcondition, ret);

    Svcs->verbose(Stream->CP_Stream,
                  "ADIOS requesting to read remote memory for Timestep %d "
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

static void RdmaWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    RdmaCompletionHandle Handle = (RdmaCompletionHandle)Handle_v;
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

static void RdmaProvideTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                struct _SstData *Data,
                                struct _SstData *LocalMetadata, long Timestep,
                                void **TimestepInfoPtr)
{
    Rdma_WS_Stream Stream = (Rdma_WS_Stream)Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));
    struct _RdmaPerTimestepInfo *Info =
        malloc(sizeof(struct _RdmaPerTimestepInfo));
    FabricState Fabric = Stream->Fabric;

    Info->CheckString = malloc(64);
    sprintf(Info->CheckString, "Rdma info for timestep %ld from rank %d",
            Timestep, Stream->Rank);
    Info->CheckInt = Stream->Rank * 1000 + Timestep;
    Entry->Data = Data;
    Entry->Timestep = Timestep;
    Entry->DP_TimestepInfo = Info;

    fi_mr_reg(Fabric->domain, Data->block, Data->DataSize, FI_REMOTE_READ, 0, 0,
              0, &Entry->mr, Fabric->ctx);
    Entry->Key = fi_mr_key(Entry->mr);
    Entry->Next = Stream->Timesteps;
    Stream->Timesteps = Entry;
    *TimestepInfoPtr = Info;
}

static void RdmaReleaseTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                long Timestep)
{
    Rdma_WS_Stream Stream = (Rdma_WS_Stream)Stream_v;
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
                fi_close((struct fid *)List->mr); // to be expanded
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

static void RdmaDestroyReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v)
{
    Rdma_RS_Stream RS_Stream = (Rdma_RS_Stream)RS_Stream_v;
    /* Stuff here */
}

static void RdmaDestroyWriter(CP_Services Svcs, DP_WS_Stream WS_Stream_v)
{
    Rdma_WS_Stream WS_Stream = (Rdma_WS_Stream)WS_Stream_v;
    /* Stuff here */
}

static void RdmaDestroyWriterPerReader(CP_Services Svcs,
                                       DP_WSR_Stream WSR_Stream_v)
{
    Rdma_WSR_Stream WSR_Stream = (Rdma_WSR_Stream)WSR_Stream_v;
    /* Stuff here */
}

static FMField RdmaReaderContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(RdmaReaderContactInfo, ContactString)},
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(RdmaReaderContactInfo, RS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaReaderContactStructs[] = {
    {"RdmaReaderContactInfo", RdmaReaderContactList,
     sizeof(struct _RdmaReaderContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField RdmaWriterContactList[] = {
    {"ContactString", "string", sizeof(char *),
     FMOffset(RdmaWriterContactInfo, ContactString)},
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(RdmaWriterContactInfo, WS_Stream)},
    {"Length", "integer", sizeof(int), FMOffset(RdmaWriterContactInfo, Length)},
    {"Address", "integer[Length]", sizeof(char),
     FMOffset(RdmaWriterContactInfo, Address)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaWriterContactStructs[] = {
    {"RdmaWriterContactInfo", RdmaWriterContactList,
     sizeof(struct _RdmaWriterContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField RdmaTimestepInfoList[] = {
    {"CheckString", "string", sizeof(char *),
     FMOffset(RdmaPerTimestepInfo, CheckString)},
    {"CheckInt", "integer", sizeof(void *),
     FMOffset(RdmaPerTimestepInfo, CheckInt)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaTimestepInfoStructs[] = {
    {"RdmaTimestepInfo", RdmaTimestepInfoList,
     sizeof(struct _RdmaPerTimestepInfo), NULL},
    {NULL, NULL, 0, NULL}};

static struct _CP_DP_Interface RdmaDPInterface;

extern CP_DP_Interface LoadRdmaDP()
{
    memset(&RdmaDPInterface, 0, sizeof(RdmaDPInterface));
    RdmaDPInterface.ReaderContactFormats = RdmaReaderContactStructs;
    RdmaDPInterface.WriterContactFormats = RdmaWriterContactStructs;
    RdmaDPInterface.TimestepInfoFormats = NULL; // RdmaTimestepInfoStructs;
    RdmaDPInterface.initReader = RdmaInitReader;
    RdmaDPInterface.initWriter = RdmaInitWriter;
    RdmaDPInterface.initWriterPerReader = RdmaInitWriterPerReader;
    RdmaDPInterface.provideWriterDataToReader = RdmaProvideWriterDataToReader;
    RdmaDPInterface.readRemoteMemory = RdmaReadRemoteMemory;
    RdmaDPInterface.waitForCompletion = RdmaWaitForCompletion;
    RdmaDPInterface.provideTimestep = RdmaProvideTimestep;
    RdmaDPInterface.releaseTimestep = RdmaReleaseTimestep;
    RdmaDPInterface.destroyReader = RdmaDestroyReader;
    RdmaDPInterface.destroyWriter = RdmaDestroyWriter;
    RdmaDPInterface.destroyWriterPerReader = RdmaDestroyWriterPerReader;
    return &RdmaDPInterface;
}
