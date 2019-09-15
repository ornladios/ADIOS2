#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adios2/common/ADIOSConfig.h"
#include <atl.h>
#include <evpath.h>

#include <SSTConfig.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>

#ifdef SST_HAVE_FI_GNI
#include <rdma/fi_ext_gni.h>
#ifdef SST_HAVE_CRAY_DRC
#include <rdmacred.h>
#endif /* SST_HAVE_CRAY_DRC */
#endif /* SST_HAVE_FI_GNI */

#include "sst_data.h"

#include "adios2/toolkit/profiling/taustubs/taustubs.h"
#include "dp_interface.h"

#define DP_AV_DEF_SIZE 512

pthread_mutex_t fabric_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wsr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ts_mutex = PTHREAD_MUTEX_INITIALIZER;

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
#ifdef SST_HAVE_CRAY_DRC
    drc_info_handle_t drc_info;
    uint32_t credential;
    struct fi_gni_auth_key *auth_key;
#endif /* SST_HAVE_CRAY_DRC */
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

static void init_fabric(struct fabric_state *fabric, struct _SstParams *Params)
{
    struct fi_info *hints, *info, *originfo, *useinfo;
    struct fi_av_attr av_attr = {0};
    struct fi_cq_attr cq_attr = {0};
    char *ifname;

    hints = fi_allocinfo();
    hints->caps = FI_MSG | FI_SEND | FI_RECV | FI_REMOTE_READ |
                  FI_REMOTE_WRITE | FI_RMA | FI_READ | FI_WRITE;
    hints->mode = FI_CONTEXT | FI_LOCAL_MR | FI_CONTEXT2 | FI_MSG_PREFIX |
                  FI_ASYNC_IOV | FI_RX_CQ_DATA;
    hints->domain_attr->mr_mode = FI_MR_BASIC;
    hints->domain_attr->control_progress = FI_PROGRESS_AUTO;
    hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
    hints->ep_attr->type = FI_EP_RDM;

    if (Params->DataInterface)
    {
        ifname = Params->DataInterface;
    }
    else
    {
        ifname = getenv("FABRIC_IFACE");
    }

    fabric->info = NULL;

    fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, hints, &info);
    if (!info)
    {
        return;
    }
    fi_freeinfo(hints);

    originfo = info;
    useinfo = NULL;
    while (info)
    {
        char *prov_name = info->fabric_attr->prov_name;
        char *domain_name = info->domain_attr->name;

        if (ifname && strcmp(ifname, domain_name) == 0)
        {
            useinfo = info;
            break;
        }
        if ((((strcmp(prov_name, "verbs") == 0) && info->src_addr) ||
             (strcmp(prov_name, "gni") == 0) ||
             (strcmp(prov_name, "psm2") == 0)) &&
            (!useinfo || !ifname ||
             (strcmp(useinfo->domain_attr->name, ifname) != 0)))
        {
            useinfo = info;
        }
        else if (((strstr(prov_name, "verbs") && info->src_addr) ||
                  strstr(prov_name, "gni") || strstr(prov_name, "psm2")) &&
                 !useinfo)
        {
            useinfo = info;
        }
        info = info->next;
    }

    info = useinfo;

    if (!info)
    {
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

    fabric->addr_len = info->src_addrlen;

    info->domain_attr->mr_mode = FI_MR_BASIC;
#ifdef SST_HAVE_CRAY_DRC
    if (strstr(info->fabric_attr->prov_name, "gni") && fabric->auth_key)
    {
        info->domain_attr->auth_key = (uint8_t *)fabric->auth_key;
        info->domain_attr->auth_key_size = sizeof(struct fi_gni_raw_auth_key);
    }
#endif /* SST_HAVE_CRAY_DRC */
    fabric->info = fi_dupinfo(info);

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

    fi_freeinfo(originfo);
}

static void fini_fabric(struct fabric_state *fabric)
{

    int status;

    do
    {
        status = fi_close((struct fid *)fabric->cq_signal);
    } while (status == FI_EBUSY);

    fi_close((struct fid *)fabric->domain);
    fi_close((struct fid *)fabric->fabric);

    if (status)
    {
        // TODO: error handling
    }

    fi_freeinfo(fabric->info);

    if (fabric->ctx)
    {
        free(fabric->ctx);
    }
}

typedef struct fabric_state *FabricState;

typedef struct _Rdma_RS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;
    FabricState Fabric;
    struct _SstParams *Params;

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
    struct _RdmaWriterContactInfo *WriterContactInfo;
} * Rdma_WSR_Stream;

typedef struct _RdmaPerTimestepInfo
{
    uint8_t *Block;
    uint64_t Key;
} * RdmaPerTimestepInfo;

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

    int ReaderCount;
    Rdma_WSR_Stream *Readers;
} * Rdma_WS_Stream;

typedef struct _RdmaReaderContactInfo
{
    void *RS_Stream;
} * RdmaReaderContactInfo;

typedef struct _RdmaWriterContactInfo
{
    void *WS_Stream;
    size_t Length;
    void *Address;
#ifdef SST_HAVE_CRAY_DRC
    int Credential;
#endif /* SST_HAVE_CRAY_DRC */
} * RdmaWriterContactInfo;

static DP_RS_Stream RdmaInitReader(CP_Services Svcs, void *CP_Stream,
                                   void **ReaderContactInfoPtr,
                                   struct _SstParams *Params,
                                   attr_list WriterContact)
{
    Rdma_RS_Stream Stream = malloc(sizeof(struct _Rdma_RS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    FabricState Fabric;

    memset(Stream, 0, sizeof(*Stream));

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    MPI_Comm_rank(comm, &Stream->Rank);

    *ReaderContactInfoPtr = NULL;

    if (Params)
    {
        Stream->Params = malloc(sizeof(*Stream->Params));
        memcpy(Stream->Params, Params, sizeof(*Params));
    }

#ifdef SST_HAVE_CRAY_DRC
    int protection_key;
    if (!get_int_attr(WriterContact, attr_atom_from_string("RDMA_DRC_KEY"),
                      &protection_key))
    {
        Svcs->verbose(CP_Stream, "Didn't find DRC credential\n");
        return NULL;
    }
    // do something with protection key?
#endif
    return Stream;
}

typedef struct _RdmaCompletionHandle
{
    struct fid_mr *LocalMR;
    void *CPStream;
    void *Buffer;
    size_t Length;
    int Rank;
    int Pending;
    double StartWTime;
} * RdmaCompletionHandle;

static DP_WS_Stream RdmaInitWriter(CP_Services Svcs, void *CP_Stream,
                                   struct _SstParams *Params, attr_list DPAttrs)
{
    Rdma_WS_Stream Stream = malloc(sizeof(struct _Rdma_WS_Stream));
    CManager cm = Svcs->getCManager(CP_Stream);
    MPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    FabricState Fabric;
    int rc;

    memset(Stream, 0, sizeof(struct _Rdma_WS_Stream));

    MPI_Comm_rank(comm, &Stream->Rank);

    Stream->Fabric = calloc(1, sizeof(struct fabric_state));
    Fabric = Stream->Fabric;
#ifdef SST_HAVE_CRAY_DRC
    if (Stream->Rank == 0)
    {
        rc = drc_acquire(&Fabric->credential, DRC_FLAGS_FLEX_CREDENTIAL);
        if (rc != DRC_SUCCESS)
        {
            Svcs->verbose(CP_Stream,
                          "Could not acquire DRC credential. Failed with %d.\n",
                          rc);
            goto err_out;
        }
        else
        {
            Svcs->verbose(CP_Stream, "DRC acquired credential id %d.\n",
                          Fabric->credential);
        }
    }

    MPI_Bcast(&Fabric->credential, sizeof(Fabric->credential), MPI_BYTE, 0,
              comm);
    rc = drc_access(Fabric->credential, 0, &Fabric->drc_info);
    if (rc != DRC_SUCCESS)
    {
        Svcs->verbose(CP_Stream,
                      "Could not access DRC credential. Failed with %d.\n", rc);
        goto err_out;
    }

    Fabric->auth_key = malloc(sizeof(*Fabric->auth_key));
    Fabric->auth_key->type = GNIX_AKT_RAW;
    Fabric->auth_key->raw.protection_key =
        drc_get_first_cookie(Fabric->drc_info);
    Svcs->verbose(CP_Stream, "Using protection key %08x.\n",
                  Fabric->auth_key->raw.protection_key);

    set_int_attr(DPAttrs, attr_atom_from_string("RDMA_DRC_KEY"),
                 Fabric->auth_key->raw.protection_key);

#endif /* SST_HAVE_CRAY_DRC */

    init_fabric(Stream->Fabric, Params);
    Fabric = Stream->Fabric;
    if (!Fabric->info)
    {
        Svcs->verbose(CP_Stream, "Could not find a valid transport fabric.\n");
        goto err_out;
    }

    Svcs->verbose(CP_Stream, "Fabric Parameters:\n%s\n",
                  fi_tostr(Fabric->info, FI_TYPE_INFO));

    /*
     * save the CP_stream value of later use
     */
    Stream->CP_Stream = CP_Stream;

    return (void *)Stream;

err_out:
    if (Stream)
    {
        if (Stream->Fabric)
        {
            free(Stream->Fabric);
        }
        free(Stream);
    }
    return (NULL);
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

    MPI_Comm_rank(comm, &Rank);

    WSR_Stream->WS_Stream = WS_Stream; /* pointer to writer struct */
    WSR_Stream->PeerCohort = PeerCohort;

    WSR_Stream->ReaderCohortSize = readerCohortSize;

    /*
     * add this writer-side reader-specific stream to the parent writer stream
     * structure
     */
    pthread_mutex_lock(&wsr_mutex);
    WS_Stream->Readers = realloc(
        WS_Stream->Readers, sizeof(*WSR_Stream) * (WS_Stream->ReaderCount + 1));
    WS_Stream->Readers[WS_Stream->ReaderCount] = WSR_Stream;
    WS_Stream->ReaderCount++;
    pthread_mutex_unlock(&wsr_mutex);

    ContactInfo = malloc(sizeof(struct _RdmaWriterContactInfo));
    memset(ContactInfo, 0, sizeof(struct _RdmaWriterContactInfo));
    ContactInfo->WS_Stream = WSR_Stream;

    ContactInfo->Length = Fabric->info->src_addrlen;
    ContactInfo->Address = malloc(ContactInfo->Length);
    fi_getname((fid_t)Fabric->signal, ContactInfo->Address,
               &ContactInfo->Length);
#ifdef SST_HAVE_CRAY_DRC
    ContactInfo->Credential = Fabric->credential;
#endif /* SST_HAVE_CRAY_DRC */
    WSR_Stream->WriterContactInfo = ContactInfo;
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
    void *CP_Stream = RS_Stream->CP_Stream;
    int rc;

    RS_Stream->PeerCohort = PeerCohort;
    RS_Stream->WriterCohortSize = writerCohortSize;
    RS_Stream->WriterAddr =
        calloc(writerCohortSize, sizeof(*RS_Stream->WriterAddr));

    RS_Stream->Fabric = calloc(1, sizeof(struct fabric_state));

    Fabric = RS_Stream->Fabric;
#ifdef SST_HAVE_CRAY_DRC
    if (providedWriterInfo)
    {
        Fabric->credential = (*providedWriterInfo)->Credential;
    }
    else
    {
        Svcs->verbose(CP_Stream,
                      "Writer contact info needed to access DRC credentials.\n",
                      rc);
    }

    rc = drc_access(Fabric->credential, 0, &Fabric->drc_info);
    if (rc != DRC_SUCCESS)
    {
        Svcs->verbose(CP_Stream,
                      "Could not access DRC credential. Failed with %d.\n", rc);
    }

    Fabric->auth_key = malloc(sizeof(*Fabric->auth_key));
    Fabric->auth_key->type = GNIX_AKT_RAW;
    Fabric->auth_key->raw.protection_key =
        drc_get_first_cookie(Fabric->drc_info);
    Svcs->verbose(CP_Stream, "Using protection key %08x.\n",
                  Fabric->auth_key->raw.protection_key);
#endif /* SST_HAVE_CRAY_DRC */

    init_fabric(RS_Stream->Fabric, RS_Stream->Params);
    if (!Fabric->info)
    {
        Svcs->verbose(CP_Stream, "Could not find a valid transport fabric.\n");
    }

    Svcs->verbose(CP_Stream, "Fabric Parameters:\n%s\n",
                  fi_tostr(Fabric->info, FI_TYPE_INFO));

    /*
     * make a copy of writer contact information (original will not be
     * preserved)
     */
    RS_Stream->WriterContactInfo =
        malloc(sizeof(struct _RdmaWriterContactInfo) * writerCohortSize);
    for (int i = 0; i < writerCohortSize; i++)
    {
        RS_Stream->WriterContactInfo[i].WS_Stream =
            providedWriterInfo[i]->WS_Stream;
        fi_av_insert(Fabric->av, providedWriterInfo[i]->Address, 1,
                     &RS_Stream->WriterAddr[i], 0, NULL);
        Svcs->verbose(RS_Stream->CP_Stream,
                      "Received contact info for WS_stream %p, WSR Rank %d\n",
                      RS_Stream->WriterContactInfo[i].WS_Stream, i);
    }
}

static void *RdmaReadRemoteMemory(CP_Services Svcs, DP_RS_Stream Stream_v,
                                  int Rank, long Timestep, size_t Offset,
                                  size_t Length, void *Buffer,
                                  void *DP_TimestepInfo)
{
    Rdma_RS_Stream RS_Stream = (Rdma_RS_Stream)Stream_v;
    FabricState Fabric = RS_Stream->Fabric;
    RdmaPerTimestepInfo Info = (RdmaPerTimestepInfo)DP_TimestepInfo;
    RdmaCompletionHandle ret = malloc(sizeof(struct _RdmaCompletionHandle));
    fi_addr_t SrcAddress;
    void *LocalDesc = NULL;
    uint8_t *Addr;
    ssize_t rc;

    Svcs->verbose(RS_Stream->CP_Stream,
                  "Performing remote read of Writer Rank %d\n", Rank);

    if (Info)
    {
        Svcs->verbose(RS_Stream->CP_Stream,
                      "Block address is %p, with a key of %d\n", Info->Block,
                      Info->Key);
    }
    else
    {
        Svcs->verbose(RS_Stream->CP_Stream, "Timestep info is null\n");
    }

    ret->CPStream = RS_Stream;
    ret->Buffer = Buffer;
    ret->Rank = Rank;
    ret->Length = Length;
    ret->Pending = 1;

    SrcAddress = RS_Stream->WriterAddr[Rank];

    if (Fabric->local_mr_req)
    {
        // register dest buffer
        fi_mr_reg(Fabric->domain, Buffer, Length, FI_READ, 0, 0, 0,
                  &ret->LocalMR, Fabric->ctx);
        LocalDesc = fi_mr_desc(ret->LocalMR);
    }

    Addr = Info->Block + Offset;

    Svcs->verbose(RS_Stream->CP_Stream,
                  "Target of remote read on Writer Rank %d is %p\n", Rank,
                  Addr);

    do
    {
        ret->StartWTime = MPI_Wtime();
        rc = fi_read(Fabric->signal, Buffer, Length, LocalDesc, SrcAddress,
                     (uint64_t)Addr, Info->Key, ret);
    } while (rc == -EAGAIN);

    if (rc != 0)
    {
        Svcs->verbose(RS_Stream->CP_Stream, "fi_read failed with code %d.\n",
                      rc);
        free(ret);
        return NULL;
    }

    Svcs->verbose(RS_Stream->CP_Stream,
                  "Posted RDMA get for Writer Rank %d for handle %p\n", Rank,
                  (void *)ret);

    return (ret);
}

static void RdmaNotifyConnFailure(CP_Services Svcs, DP_RS_Stream Stream_v,
                                  int FailedPeerRank)
{
    Rdma_RS_Stream Stream = (Rdma_RS_Stream)
        Stream_v; /* DP_RS_Stream is the return from InitReader */
    CManager cm = Svcs->getCManager(Stream->CP_Stream);
    Svcs->verbose(Stream->CP_Stream,
                  "received notification that writer peer "
                  "%d has failed, failing any pending "
                  "requests\n",
                  FailedPeerRank);
    //   This is what EVPath does...
    //   FailRequestsToRank(Svcs, cm, Stream, FailedPeerRank);
}

/*
 * RdmaWaitForCompletion should return 1 if successful, but 0 if the reads
 * failed for some reason or were aborted by RdmaNotifyConnFailure()
 */
static int RdmaWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    RdmaCompletionHandle Handle = (RdmaCompletionHandle)Handle_v;
    Rdma_RS_Stream Stream = Handle->CPStream;
    FabricState Fabric = Stream->Fabric;
    RdmaCompletionHandle Handle_t;
    struct fi_cq_data_entry CQEntry = {0};

    while (Handle->Pending > 0)
    {
        ssize_t rc;
        rc = fi_cq_sread(Fabric->cq_signal, (void *)(&CQEntry), 1, NULL, -1);
        if (rc < 1)
        {
            // Handle errrors
        }
        else
        {
            Svcs->verbose(Stream->CP_Stream,
                          "got completion for request with handle %p.\n",
                          CQEntry.op_context);
            Handle_t = (RdmaCompletionHandle)CQEntry.op_context;
            Handle_t->Pending--;
        }
    }

    if (Fabric->local_mr_req)
    {
        fi_close((struct fid *)Handle->LocalMR);
    }

    return (1);
}

static void RdmaProvideTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                struct _SstData *Data,
                                struct _SstData *LocalMetadata, long Timestep,
                                void **TimestepInfoPtr)
{
    Rdma_WS_Stream Stream = (Rdma_WS_Stream)Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));
    RdmaPerTimestepInfo Info = malloc(sizeof(struct _RdmaPerTimestepInfo));
    FabricState Fabric = Stream->Fabric;

    Entry->Data = malloc(sizeof(*Data));
    memcpy(Entry->Data, Data, sizeof(*Data));
    Entry->Timestep = Timestep;
    Entry->DP_TimestepInfo = Info;

    fi_mr_reg(Fabric->domain, Data->block, Data->DataSize, FI_REMOTE_READ, 0, 0,
              0, &Entry->mr, Fabric->ctx);
    Entry->Key = fi_mr_key(Entry->mr);
    pthread_mutex_lock(&ts_mutex);
    Entry->Next = Stream->Timesteps;
    Stream->Timesteps = Entry;
    // Probably doesn't need to be in the lock
    // |
    // ---------------------------------------------------------------------------------------------------
    Info->Key = Entry->Key;
    pthread_mutex_unlock(&ts_mutex);
    Info->Block = (uint8_t *)Data->block;

    Svcs->verbose(Stream->CP_Stream,
                  "Providing timestep data with block %p and access key %d\n",
                  Info->Block, Info->Key);

    *TimestepInfoPtr = Info;
}

static void RdmaReleaseTimestep(CP_Services Svcs, DP_WS_Stream Stream_v,
                                long Timestep)
{
    Rdma_WS_Stream Stream = (Rdma_WS_Stream)Stream_v;
    TimestepList *List = &Stream->Timesteps;
    TimestepList ReleaseTSL;
    RdmaPerTimestepInfo Info;

    Svcs->verbose(Stream->CP_Stream, "Releasing timestep %ld\n", Timestep);

    pthread_mutex_lock(&ts_mutex);
    while ((*List) && (*List)->Timestep != Timestep)
    {
        List = &((*List)->Next);
    }

    if ((*List) == NULL)
    {
        /*
         * Shouldn't ever get here because we should never release a
         * timestep that we don't have.
         */
        fprintf(stderr, "Failed to release Timestep %ld, not found\n",
                Timestep);
        assert(0);
    }

    ReleaseTSL = *List;
    *List = ReleaseTSL->Next;
    pthread_mutex_unlock(&ts_mutex);
    fi_close((struct fid *)ReleaseTSL->mr);
    if (ReleaseTSL->Data)
    {
        free(ReleaseTSL->Data);
    }
    Info = ReleaseTSL->DP_TimestepInfo;
    if (Info)
    {
        free(Info);
    }
    free(ReleaseTSL);
}

static void RdmaDestroyReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v)
{
    Rdma_RS_Stream RS_Stream = (Rdma_RS_Stream)RS_Stream_v;

    Svcs->verbose(RS_Stream->CP_Stream, "Tearing down RDMA state on reader.\n");
    fini_fabric(RS_Stream->Fabric);

    free(RS_Stream->WriterContactInfo);
    free(RS_Stream->WriterAddr);
    free(RS_Stream->Fabric);
    free(RS_Stream);
}

static void RdmaDestroyWriterPerReader(CP_Services Svcs,
                                       DP_WSR_Stream WSR_Stream_v)
{
    Rdma_WSR_Stream WSR_Stream = (Rdma_WSR_Stream)WSR_Stream_v;
    Rdma_WS_Stream WS_Stream = WSR_Stream->WS_Stream;
    RdmaWriterContactInfo WriterContactInfo;

    pthread_mutex_lock(&wsr_mutex);
    for (int i = 0; i < WS_Stream->ReaderCount; i++)
    {
        if (WS_Stream->Readers[i] == WSR_Stream)
        {
            WS_Stream->Readers[i] =
                WS_Stream->Readers[WS_Stream->ReaderCount - 1];
            break;
        }
    }
    WS_Stream->Readers = realloc(
        WS_Stream->Readers, sizeof(*WSR_Stream) * (WS_Stream->ReaderCount - 1));
    WS_Stream->ReaderCount--;
    pthread_mutex_unlock(&wsr_mutex);

    if (WSR_Stream->WriterContactInfo)
    {
        WriterContactInfo = WSR_Stream->WriterContactInfo;
        free(WriterContactInfo->Address);
    }
    free(WSR_Stream->WriterContactInfo);
    free(WSR_Stream);
}

static FMField RdmaReaderContactList[] = {
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(RdmaReaderContactInfo, RS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaReaderContactStructs[] = {
    {"RdmaReaderContactInfo", RdmaReaderContactList,
     sizeof(struct _RdmaReaderContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static void RdmaDestroyWriter(CP_Services Svcs, DP_WS_Stream WS_Stream_v)
{
    Rdma_WS_Stream WS_Stream = (Rdma_WS_Stream)WS_Stream_v;
    TimestepList List;
#ifdef SST_HAVE_CRAY_DRC
    uint32_t Credential;

    Credential = WS_Stream->Fabric->credential;
#endif /* SST_HAVE_CRAY_DRC */

    Svcs->verbose(WS_Stream->CP_Stream, "Tearing down RDMA state on writer.\n");
    fini_fabric(WS_Stream->Fabric);

#ifdef SST_HAVE_CRAY_DRC
    if (WS_Stream->Rank == 0)
    {
        drc_release(Credential, 0);
    }
#endif /* SST_HAVE_CRAY_DRC */

    while (WS_Stream->ReaderCount > 0)
    {
        RdmaDestroyWriterPerReader(Svcs, WS_Stream->Readers[0]);
    }

    pthread_mutex_lock(&ts_mutex);
    while (WS_Stream->Timesteps)
    {
        List = WS_Stream->Timesteps;
        RdmaReleaseTimestep(Svcs, WS_Stream, List->Timestep);
    }
    pthread_mutex_unlock(&ts_mutex);

    free(WS_Stream->Fabric);
    free(WS_Stream);
}

static FMField RdmaWriterContactList[] = {
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(RdmaWriterContactInfo, WS_Stream)},
    {"Length", "integer", sizeof(int), FMOffset(RdmaWriterContactInfo, Length)},
    {"Address", "integer[Length]", sizeof(char),
     FMOffset(RdmaWriterContactInfo, Address)},
#ifdef SST_HAVE_CRAY_DRC
    {"Credential", "integer", sizeof(int),
     FMOffset(RdmaWriterContactInfo, Credential)},
#endif /* SST_HAVE_CRAY_DRC */
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaWriterContactStructs[] = {
    {"RdmaWriterContactInfo", RdmaWriterContactList,
     sizeof(struct _RdmaWriterContactInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField RdmaTimestepInfoList[] = {
    {"Block", "integer", sizeof(void *), FMOffset(RdmaPerTimestepInfo, Block)},
    {"Key", "integer", sizeof(uint64_t), FMOffset(RdmaPerTimestepInfo, Key)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec RdmaTimestepInfoStructs[] = {
    {"RdmaTimestepInfo", RdmaTimestepInfoList,
     sizeof(struct _RdmaPerTimestepInfo), NULL},
    {NULL, NULL, 0, NULL}};

static struct _CP_DP_Interface RdmaDPInterface;

/* In RdmaGetPriority, the Rdma DP should do whatever is necessary to test to
 * see if it
 * *can* run, and then return -1 if it cannot run and a value
 * greater than 1 if it can.  Eventually if we have more than one
 * possible RDMA DP, we may need some better scheme, maybe where the
 * "priority" return value represents some desirability measure
 * (like expected bandwidth or something)
 *
 * Returns 10 if a valid RDMA fabric is found, 100 if that fabric matches
 * what is set in the FABRIC_IFACE environment variable.
 *
 */
static int RdmaGetPriority(CP_Services Svcs, void *CP_Stream,
                           struct _SstParams *Params)
{
    struct fi_info *hints, *info, *originfo, *useinfo;
    char *ifname;
    int Ret = -1;

    (void)attr_atom_from_string(
        "RDMA_DRC_KEY"); // make sure this attribute is translatable

    hints = fi_allocinfo();
    hints->caps = FI_MSG | FI_SEND | FI_RECV | FI_REMOTE_READ |
                  FI_REMOTE_WRITE | FI_RMA | FI_READ | FI_WRITE;
    hints->mode = FI_CONTEXT | FI_LOCAL_MR | FI_CONTEXT2 | FI_MSG_PREFIX |
                  FI_ASYNC_IOV | FI_RX_CQ_DATA;
    hints->domain_attr->mr_mode = FI_MR_BASIC;
    hints->domain_attr->control_progress = FI_PROGRESS_AUTO;
    hints->domain_attr->data_progress = FI_PROGRESS_AUTO;
    hints->ep_attr->type = FI_EP_RDM;

    if (Params->DataInterface)
    {
        ifname = Params->DataInterface;
    }
    else
    {
        ifname = getenv("FABRIC_IFACE");
    }

    fi_getinfo(FI_VERSION(1, 5), NULL, NULL, 0, hints, &info);
    fi_freeinfo(hints);

    if (!info)
    {
        Svcs->verbose(CP_Stream,
                      "RDMA Dataplane could not find any viable fabrics.\n");
    }

    originfo = info;

    while (info)
    {
        char *prov_name, *domain_name;

        prov_name = info->fabric_attr->prov_name;
        domain_name = info->domain_attr->name;
        if (ifname && strcmp(ifname, domain_name) == 0)
        {
            Svcs->verbose(CP_Stream,
                          "RDMA Dataplane found the requested "
                          "interface %s, provider type %s.\n",
                          ifname, prov_name);
            Ret = 100;
            break;
        }
        if ((strstr(prov_name, "verbs") && info->src_addr) ||
            strstr(prov_name, "gni") || strstr(prov_name, "psm2"))
        {

            Svcs->verbose(CP_Stream,
                          "RDMA Dataplane sees interface %s, "
                          "provider type %s, which should work.\n",
                          domain_name, prov_name);
            Ret = 10;
        }
        info = info->next;
    }

    if (Ret == -1)
    {
        Svcs->verbose(
            CP_Stream,
            "RDMA Dataplane could not find an RDMA-compatible fabric.\n");
    }

    if (originfo)
    {
        fi_freeinfo(originfo);
    }

    Svcs->verbose(
        CP_Stream,
        "RDMA Dataplane evaluating viability, returning priority %d\n", Ret);
    return Ret;
}

/* If RdmaGetPriority has allocated resources or initialized something
 *  that needs to be cleaned up, RdmaUnGetPriority should undo that
 * operation.
 */
static void RdmaUnGetPriority(CP_Services Svcs, void *CP_Stream)
{
    Svcs->verbose(CP_Stream, "RDMA Dataplane unloading\n");
}

extern CP_DP_Interface LoadRdmaDP()
{
    memset(&RdmaDPInterface, 0, sizeof(RdmaDPInterface));
    RdmaDPInterface.ReaderContactFormats = RdmaReaderContactStructs;
    RdmaDPInterface.WriterContactFormats = RdmaWriterContactStructs;
    RdmaDPInterface.TimestepInfoFormats = RdmaTimestepInfoStructs;
    RdmaDPInterface.initReader = RdmaInitReader;
    RdmaDPInterface.initWriter = RdmaInitWriter;
    RdmaDPInterface.initWriterPerReader = RdmaInitWriterPerReader;
    RdmaDPInterface.provideWriterDataToReader = RdmaProvideWriterDataToReader;
    RdmaDPInterface.readRemoteMemory = RdmaReadRemoteMemory;
    RdmaDPInterface.waitForCompletion = RdmaWaitForCompletion;
    RdmaDPInterface.notifyConnFailure = RdmaNotifyConnFailure;
    RdmaDPInterface.provideTimestep = RdmaProvideTimestep;
    RdmaDPInterface.readerRegisterTimestep = NULL;
    RdmaDPInterface.releaseTimestep = RdmaReleaseTimestep;
    RdmaDPInterface.readerReleaseTimestep = NULL;
    RdmaDPInterface.destroyReader = RdmaDestroyReader;
    RdmaDPInterface.destroyWriter = RdmaDestroyWriter;
    RdmaDPInterface.destroyWriterPerReader = RdmaDestroyWriterPerReader;
    RdmaDPInterface.getPriority = RdmaGetPriority;
    RdmaDPInterface.unGetPriority = RdmaUnGetPriority;
    return &RdmaDPInterface;
}
