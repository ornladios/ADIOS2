/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Mercury-based DataPlane for SST
 *
 * This data plane uses the Mercury RPC framework for remote data access.
 * Mercury provides efficient RPC and RDMA-like bulk data transfer capabilities
 * suitable for HPC environments.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <atl.h>
#include <evpath.h>
#include <margo.h>
#include <mercury.h>
#include <mercury_macros.h>

#include "dp_interface.h"
#include "sst_data.h"
#include <adios2-perfstubs-interface.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

/*
 * Mercury DataPlane Structures
 *
 * Mercury_RS_Stream - Reader-side stream structure
 * Mercury_WS_Stream - Writer-side stream structure
 * Mercury_WSR_Stream - Writer-side per-reader stream structure
 */

/* Timing helpers */
static inline double mercury_wtime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

typedef struct _Mercury_RS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;
    SstStats Stats;

    /* Mercury runtime instance */
    margo_instance_id mid;
    hg_id_t read_rpc_id;

    /* writer info */
    int WriterCohortSize;
    CP_PeerCohort PeerCohort;
    struct _MercuryWriterContactInfo *WriterContactInfo;
    hg_addr_t *WriterAddrs; /* Mercury addresses for each writer rank */

    /* Timing stats (reader side) */
    double TimeInLookup;
    double TimeInBulkCreate;
    double TimeInRPC; /* forward + wait (includes writer-side push) */
    int LookupCount;
    int RPCCount;
} *Mercury_RS_Stream;

typedef struct _Mercury_WSR_Stream
{
    struct _Mercury_WS_Stream *WS_Stream;
    CP_PeerCohort PeerCohort;
    int ReaderCohortSize;
    struct _MercuryReaderContactInfo *ReaderContactInfo;
} *Mercury_WSR_Stream;

typedef struct _TimestepEntry
{
    size_t Timestep;
    struct _SstData *Data;
    struct _MercuryPerTimestepInfo *DP_TimestepInfo;
    struct _TimestepEntry *Next;
} *TimestepList;

typedef struct _Mercury_WS_Stream
{
    CManager cm;
    void *CP_Stream;
    int Rank;

    /* Timing stats (writer side) */
    double TimeInBulkPush;
    int PushCount;

    /* Mercury runtime instance */
    margo_instance_id mid;
    hg_id_t read_rpc_id;

    TimestepList Timesteps;

    int ReaderCount;
    Mercury_WSR_Stream *Readers;
} *Mercury_WS_Stream;

/*
 * Contact Information Structures
 *
 * These are exchanged between reader and writer sides during initialization.
 */

typedef struct _MercuryReaderContactInfo
{
    char *MercuryString; /* Mercury address string */
    void *RS_Stream;
} *MercuryReaderContactInfo;

typedef struct _MercuryWriterContactInfo
{
    char *MercuryString; /* Mercury address string */
    void *WS_Stream;
} *MercuryWriterContactInfo;

typedef struct _MercuryPerTimestepInfo
{
    char *CheckString;
    int CheckInt;
} *MercuryPerTimestepInfo;

/*
 * RPC Input/Output Structures for Mercury
 */

/* Read request RPC input - reader sends its bulk handle for the writer to push into */
MERCURY_GEN_PROC(read_request_in_t,
                 ((hg_uint64_t)(timestep))((hg_uint64_t)(offset))((hg_uint64_t)(length))(
                     (hg_uint64_t)(ws_stream))((hg_uint64_t)(rs_stream))(
                     (hg_int32_t)(requesting_rank))((hg_bulk_t)(bulk_handle)))

/* Read request RPC output */
MERCURY_GEN_PROC(read_request_out_t, ((hg_int32_t)(ret)))

/*
 * Mercury RPC Handler for Read Requests (Writer Side)
 *
 * This handler runs on the writer side when a reader requests data.
 */
static hg_return_t mercury_read_request_handler(hg_handle_t handle)
{
    PERFSTUBS_TIMER_START_FUNC(timer);
    hg_return_t hret;
    read_request_in_t in;
    read_request_out_t out;
    hg_bulk_t bulk_handle;
    const struct hg_info *hgi;
    margo_instance_id mid;

    /* Get input parameters */
    hret = margo_get_input(handle, &in);
    assert(hret == HG_SUCCESS);

    mid = margo_hg_handle_get_instance(handle);
    hgi = margo_get_info(handle);

    Mercury_WSR_Stream WSR_Stream = (Mercury_WSR_Stream)in.ws_stream;
    Mercury_WS_Stream WS_Stream = WSR_Stream->WS_Stream;

    /* Find the requested timestep data */
    TimestepList tmp = WS_Stream->Timesteps;
    while (tmp != NULL)
    {
        if (tmp->Timestep == in.timestep)
        {
            /* Create local bulk handle for the source data */
            void *data_ptr = (char *)tmp->Data->block + in.offset;
            hg_size_t bulk_size = (hg_size_t)in.length;
            hret =
                margo_bulk_create(mid, 1, &data_ptr, &bulk_size, HG_BULK_READ_ONLY, &bulk_handle);
            assert(hret == HG_SUCCESS);

            /* Push data into reader's bulk handle (standard margo pattern:
             * the side with the data initiates the transfer) */
            double t_push = mercury_wtime();
            hret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr, in.bulk_handle, 0, bulk_handle,
                                       0, bulk_size);
            WS_Stream->TimeInBulkPush += mercury_wtime() - t_push;
            WS_Stream->PushCount++;

            out.ret = (hret == HG_SUCCESS) ? 0 : -1;
            if (hret != HG_SUCCESS)
            {
                fprintf(stderr, "Mercury writer: margo_bulk_transfer (push) failed %d\n", hret);
            }

            hret = margo_respond(handle, &out);
            assert(hret == HG_SUCCESS);

            margo_bulk_free(bulk_handle);
            margo_free_input(handle, &in);
            margo_destroy(handle);

            PERFSTUBS_TIMER_STOP_FUNC(timer);
            return HG_SUCCESS;
        }
        tmp = tmp->Next;
    }

    /* Timestep not found */
    fprintf(stderr, "Failed to read Timestep %llu, not found\n", (unsigned long long)in.timestep);
    out.ret = -1;

    hret = margo_respond(handle, &out);
    margo_free_input(handle, &in);
    margo_destroy(handle);

    PERFSTUBS_TIMER_STOP_FUNC(timer);
    return HG_SUCCESS;
}
DEFINE_MARGO_RPC_HANDLER(mercury_read_request_handler)

/*
 * Completion Handle Structure
 */
typedef struct _MercuryCompletionHandle
{
    Mercury_RS_Stream RS_Stream;
    hg_handle_t handle;
    hg_bulk_t local_bulk; /* reader's bulk handle, freed after transfer */
    margo_request request;
    void *Buffer;
    size_t Length;
    int Rank;
    int Completed;
    int AsyncPending; /* 1 if async RPC is still pending */
} *MercuryCompletionHandle;

/*
 * MercuryCreateMargoInstance - Create a margo instance with CXI auth key
 * support for Slingshot networks.
 *
 * On Slingshot/CXI systems (detected via SLINGSHOT_VNIS env var), we need
 * to pass an auth key so mercury uses the correct VNI for cross-node
 * communication. When two VNIs are present, the second is the job-level
 * VNI suitable for inter-process communication.
 */
static margo_instance_id MercuryCreateMargoInstance(const char *protocol)
{
    /* On CXI/Slingshot, RDMA bulk transfers require proper auth keys.
     * margo_init_ext with JSON "auth_key":"0:0:1" lets mercury's NA OFI
     * layer auto-discover the correct VNI from SLINGSHOT_VNIS env var.
     * Plain margo_init skips this and RDMA fails with EXDEV. */
    if (strstr(protocol, "cxi") && getenv("SLINGSHOT_VNIS"))
    {
        struct margo_init_info info = {0};
        info.json_config = "{ \"mercury\": { \"auth_key\": \"0:0:1\" },"
                           "  \"use_progress_thread\": true,"
                           "  \"rpc_thread_count\": 1 }";
        return margo_init_ext(protocol, MARGO_SERVER_MODE, &info);
    }
    return margo_init(protocol, MARGO_SERVER_MODE, 1, 1);
}

/*
 * InitReader - Initialize reader-side data plane
 */
static DP_RS_Stream MercuryInitReader(CP_Services Svcs, void *CP_Stream,
                                      void **ReaderContactInfoPtr, struct _SstParams *Params,
                                      attr_list WriterContact, SstStats Stats)
{
    Mercury_RS_Stream Stream = malloc(sizeof(struct _Mercury_RS_Stream));
    MercuryReaderContactInfo Contact = malloc(sizeof(struct _MercuryReaderContactInfo));
    SMPI_Comm comm = Svcs->getMPIComm(CP_Stream);
    char *mercury_addr_str;
    hg_size_t addr_str_size = 256;

    memset(Stream, 0, sizeof(*Stream));
    memset(Contact, 0, sizeof(*Contact));

    Stream->CP_Stream = CP_Stream;
    Stream->Stats = Stats;
    SMPI_Comm_rank(comm, &Stream->Rank);

    const char *protocol = Params->MercuryProtocol;
    if (!protocol)
        protocol = getenv("SST_MERCURY_PROTOCOL");
    if (!protocol)
        protocol = "tcp";
    Stream->mid = MercuryCreateMargoInstance(protocol);
    if (Stream->mid == MARGO_INSTANCE_NULL)
    {
        Svcs->verbose(CP_Stream, DPCriticalVerbose,
                      "Failed to initialize Mercury with protocol '%s'\n", protocol);
        free(Stream);
        free(Contact);
        return NULL;
    }

    /* Get self address string */
    mercury_addr_str = malloc(addr_str_size);
    hg_addr_t self_addr;
    margo_addr_self(Stream->mid, &self_addr);
    margo_addr_to_string(Stream->mid, mercury_addr_str, &addr_str_size, self_addr);
    margo_addr_free(Stream->mid, self_addr);

    /* Register RPC */
    Stream->read_rpc_id = MARGO_REGISTER(Stream->mid, "sst_mercury_read", read_request_in_t,
                                         read_request_out_t, NULL);

    Contact->MercuryString = mercury_addr_str;
    Contact->RS_Stream = Stream;

    *ReaderContactInfoPtr = Contact;

    return Stream;
}

/*
 * DestroyReader - Cleanup reader-side resources
 */
static void MercuryDestroyReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v)
{
    Mercury_RS_Stream Stream = (Mercury_RS_Stream)RS_Stream_v;

    /* Report timing stats */
    int connected = 0;
    for (int i = 0; i < Stream->WriterCohortSize; i++)
        if (Stream->WriterAddrs[i] != HG_ADDR_NULL)
            connected++;

    Svcs->verbose(Stream->CP_Stream, DPSummaryVerbose,
                  "Mercury Reader Rank %d timing: "
                  "Lookups=%d/%d (%.4fs), BulkCreate=%d (%.4fs), "
                  "RPC+Push=%d (%.4fs)\n",
                  Stream->Rank, Stream->LookupCount, Stream->WriterCohortSize, Stream->TimeInLookup,
                  Stream->RPCCount, Stream->TimeInBulkCreate, Stream->RPCCount, Stream->TimeInRPC);

    /* Free writer addresses */
    if (Stream->WriterAddrs)
    {
        for (int i = 0; i < Stream->WriterCohortSize; i++)
        {
            if (Stream->WriterAddrs[i] != HG_ADDR_NULL)
            {
                margo_addr_free(Stream->mid, Stream->WriterAddrs[i]);
            }
        }
        free(Stream->WriterAddrs);
    }

    /* Free writer contact info */
    if (Stream->WriterContactInfo)
    {
        for (int i = 0; i < Stream->WriterCohortSize; i++)
        {
            if (Stream->WriterContactInfo[i].MercuryString)
            {
                free(Stream->WriterContactInfo[i].MercuryString);
            }
        }
        free(Stream->WriterContactInfo);
    }

    /* Finalize Mercury - this will clean up all remaining Mercury resources */
    margo_finalize_and_wait(Stream->mid);

    free(Stream);
}

/*
 * InitWriter - Initialize writer-side data plane
 */
static DP_WS_Stream MercuryInitWriter(CP_Services Svcs, void *CP_Stream, struct _SstParams *Params,
                                      attr_list DPAttrs, SstStats Stats)
{
    Mercury_WS_Stream Stream = malloc(sizeof(struct _Mercury_WS_Stream));
    SMPI_Comm comm = Svcs->getMPIComm(CP_Stream);

    memset(Stream, 0, sizeof(*Stream));

    Stream->CP_Stream = CP_Stream;
    SMPI_Comm_rank(comm, &Stream->Rank);

    const char *protocol = Params->MercuryProtocol;
    if (!protocol)
        protocol = getenv("SST_MERCURY_PROTOCOL");
    if (!protocol)
        protocol = "tcp";
    Stream->mid = MercuryCreateMargoInstance(protocol);
    if (Stream->mid == MARGO_INSTANCE_NULL)
    {
        Svcs->verbose(CP_Stream, DPCriticalVerbose,
                      "Failed to initialize Mercury with protocol '%s'\n", protocol);
        free(Stream);
        return NULL;
    }

    /* Register RPC handler */
    Stream->read_rpc_id = MARGO_REGISTER(Stream->mid, "sst_mercury_read", read_request_in_t,
                                         read_request_out_t, mercury_read_request_handler);

    return Stream;
}

/*
 * DestroyWriter - Cleanup writer-side resources
 */
static void MercuryDestroyWriter(CP_Services Svcs, DP_WS_Stream WS_Stream_v)
{
    Mercury_WS_Stream Stream = (Mercury_WS_Stream)WS_Stream_v;

    /* Report timing stats */
    Svcs->verbose(Stream->CP_Stream, DPSummaryVerbose,
                  "Mercury Writer Rank %d timing: BulkPush=%d (%.4fs)\n", Stream->Rank,
                  Stream->PushCount, Stream->TimeInBulkPush);

    /* Free timestep list */
    TimestepList tmp = Stream->Timesteps;
    while (tmp != NULL)
    {
        TimestepList next = tmp->Next;
        if (tmp->DP_TimestepInfo)
            free(tmp->DP_TimestepInfo);
        free(tmp);
        tmp = next;
    }

    free(Stream->Readers);

    /* Finalize Mercury */
    margo_finalize_and_wait(Stream->mid);

    free(Stream);
}

/*
 * InitWriterPerReader - Initialize per-reader writer-side structures
 */
static DP_WSR_Stream MercuryInitWriterPerReader(CP_Services Svcs, DP_WS_Stream WS_Stream_v,
                                                int ReaderCohortSize, CP_PeerCohort PeerCohort,
                                                void **ProvidedReaderInfo,
                                                void **WriterContactInfoPtr)
{
    Mercury_WS_Stream WS_Stream = (Mercury_WS_Stream)WS_Stream_v;
    Mercury_WSR_Stream WSR_Stream = malloc(sizeof(struct _Mercury_WSR_Stream));
    MercuryWriterContactInfo Contact = malloc(sizeof(struct _MercuryWriterContactInfo));
    char *mercury_addr_str;
    hg_size_t addr_str_size = 256;

    memset(WSR_Stream, 0, sizeof(*WSR_Stream));

    WSR_Stream->WS_Stream = WS_Stream;
    WSR_Stream->PeerCohort = PeerCohort;
    WSR_Stream->ReaderCohortSize = ReaderCohortSize;

    /* Get self address */
    mercury_addr_str = malloc(addr_str_size);
    hg_addr_t self_addr_wsr;
    margo_addr_self(WS_Stream->mid, &self_addr_wsr);
    margo_addr_to_string(WS_Stream->mid, mercury_addr_str, &addr_str_size, self_addr_wsr);
    margo_addr_free(WS_Stream->mid, self_addr_wsr);

    Contact->MercuryString = mercury_addr_str;
    Contact->WS_Stream = WSR_Stream;

    /* Deep-copy reader contact info (no address lookup needed — writer only
     * responds to incoming RPCs, never initiates outbound to readers) */
    WSR_Stream->ReaderContactInfo =
        calloc(ReaderCohortSize, sizeof(struct _MercuryReaderContactInfo));

    for (int i = 0; i < ReaderCohortSize; i++)
    {
        MercuryReaderContactInfo reader_info = (MercuryReaderContactInfo)ProvidedReaderInfo[i];
        WSR_Stream->ReaderContactInfo[i].MercuryString = strdup(reader_info->MercuryString);
        WSR_Stream->ReaderContactInfo[i].RS_Stream = reader_info->RS_Stream;
    }

    *WriterContactInfoPtr = Contact;

    /* Add to writer's reader list */
    WS_Stream->Readers =
        realloc(WS_Stream->Readers, sizeof(*WS_Stream->Readers) * (WS_Stream->ReaderCount + 1));
    WS_Stream->Readers[WS_Stream->ReaderCount] = WSR_Stream;
    WS_Stream->ReaderCount++;

    return WSR_Stream;
}

/*
 * DestroyWriterPerReader - Cleanup per-reader writer-side resources
 */
static void MercuryDestroyWriterPerReader(CP_Services Svcs, DP_WSR_Stream WSR_Stream_v)
{
    Mercury_WSR_Stream WSR_Stream = (Mercury_WSR_Stream)WSR_Stream_v;

    /* Clean up mercury's internally-cached peer addresses from incoming
     * RPC connections.  Mercury caches the source address when it receives
     * an RPC, and these must be freed before margo_finalize. */
    if (WSR_Stream->ReaderContactInfo)
    {
        for (int i = 0; i < WSR_Stream->ReaderCohortSize; i++)
        {
            if (WSR_Stream->ReaderContactInfo[i].MercuryString)
            {
                hg_addr_t peer_addr;
                hg_return_t hret =
                    margo_addr_lookup(WSR_Stream->WS_Stream->mid,
                                      WSR_Stream->ReaderContactInfo[i].MercuryString, &peer_addr);
                if (hret == HG_SUCCESS)
                {
                    margo_addr_set_remove(WSR_Stream->WS_Stream->mid, peer_addr);
                    margo_addr_free(WSR_Stream->WS_Stream->mid, peer_addr);
                }
            }
        }
    }

    /* Free reader contact info */
    if (WSR_Stream->ReaderContactInfo)
    {
        for (int i = 0; i < WSR_Stream->ReaderCohortSize; i++)
        {
            if (WSR_Stream->ReaderContactInfo[i].MercuryString)
            {
                free(WSR_Stream->ReaderContactInfo[i].MercuryString);
            }
        }
        free(WSR_Stream->ReaderContactInfo);
    }

    free(WSR_Stream);
}

/*
 * ProvideWriterDataToReader - Provide writer contact info to reader
 */
static void MercuryProvideWriterDataToReader(CP_Services Svcs, DP_RS_Stream RS_Stream_v,
                                             int WriterCohortSize, CP_PeerCohort PeerCohort,
                                             void **ProvidedWriterInfo)
{
    Mercury_RS_Stream RS_Stream = (Mercury_RS_Stream)RS_Stream_v;

    RS_Stream->PeerCohort = PeerCohort;
    RS_Stream->WriterCohortSize = WriterCohortSize;
    RS_Stream->WriterContactInfo =
        calloc(WriterCohortSize, sizeof(struct _MercuryWriterContactInfo));
    /* WriterAddrs initialized to HG_ADDR_NULL (zero) via calloc —
     * actual address lookup is deferred to first use in ReadRemoteMemory */
    RS_Stream->WriterAddrs = calloc(WriterCohortSize, sizeof(hg_addr_t));

    /* Deep-copy writer contact info only — no address lookups here */
    for (int i = 0; i < WriterCohortSize; i++)
    {
        MercuryWriterContactInfo writer_info = (MercuryWriterContactInfo)ProvidedWriterInfo[i];
        RS_Stream->WriterContactInfo[i].MercuryString = strdup(writer_info->MercuryString);
        RS_Stream->WriterContactInfo[i].WS_Stream = writer_info->WS_Stream;
    }
}

/*
 * ReadRemoteMemory - Initiate remote memory read using Mercury RPC
 */
static void *MercuryReadRemoteMemory(CP_Services Svcs, DP_RS_Stream RS_Stream_v, int Rank,
                                     size_t Timestep, size_t Offset, size_t Length, void *Buffer,
                                     void *DP_TimestepInfo)
{
    Mercury_RS_Stream RS_Stream = (Mercury_RS_Stream)RS_Stream_v;
    MercuryCompletionHandle Handle = malloc(sizeof(struct _MercuryCompletionHandle));
    hg_return_t hret;
    read_request_in_t in;

    /* Store context for WaitForCompletion */
    Handle->RS_Stream = RS_Stream;
    Handle->Buffer = Buffer;
    Handle->Length = Length;
    Handle->Rank = Rank;
    Handle->Completed = 0;
    Handle->AsyncPending = 1;
    Handle->local_bulk = HG_BULK_NULL;

    /* Lazy address lookup — only connect to writers we actually read from */
    if (RS_Stream->WriterAddrs[Rank] == HG_ADDR_NULL)
    {
        double t_lookup = mercury_wtime();
        hret = margo_addr_lookup(RS_Stream->mid, RS_Stream->WriterContactInfo[Rank].MercuryString,
                                 &RS_Stream->WriterAddrs[Rank]);
        RS_Stream->TimeInLookup += mercury_wtime() - t_lookup;
        RS_Stream->LookupCount++;
        assert(hret == HG_SUCCESS);
    }

    /* Create RPC handle */
    Handle->handle = HG_HANDLE_NULL;
    hret = margo_create(RS_Stream->mid, RS_Stream->WriterAddrs[Rank], RS_Stream->read_rpc_id,
                        &Handle->handle);
    assert(hret == HG_SUCCESS);

    /* Create bulk handle for the receive buffer — the writer will push
     * data into this buffer (standard margo RDMA pattern) */
    double t_bulk = mercury_wtime();
    hg_size_t hg_length = (hg_size_t)Length;
    hg_bulk_t local_bulk;
    hret =
        margo_bulk_create(RS_Stream->mid, 1, &Buffer, &hg_length, HG_BULK_WRITE_ONLY, &local_bulk);
    RS_Stream->TimeInBulkCreate += mercury_wtime() - t_bulk;
    assert(hret == HG_SUCCESS);

    /* Set up RPC input — include our bulk handle for the writer to push into */
    in.timestep = Timestep;
    in.offset = Offset;
    in.length = Length;
    in.ws_stream = (hg_uint64_t)RS_Stream->WriterContactInfo[Rank].WS_Stream;
    in.rs_stream = (hg_uint64_t)RS_Stream;
    in.requesting_rank = RS_Stream->Rank;
    in.bulk_handle = local_bulk;
    Handle->local_bulk = local_bulk;

    /* Forward RPC asynchronously - return immediately */
    hret = margo_iforward(Handle->handle, &in, &Handle->request);
    assert(hret == HG_SUCCESS);

    /* Return immediately - actual data retrieval and transfer happens in WaitForCompletion */
    return Handle;
}

/*
 * WaitForCompletion - Wait for remote memory read to complete
 */
static int MercuryWaitForCompletion(CP_Services Svcs, void *Handle_v)
{
    MercuryCompletionHandle Handle = (MercuryCompletionHandle)Handle_v;
    hg_return_t hret;
    read_request_out_t out;
    int ret = 0;

    if (!Handle || !Handle->AsyncPending)
    {
        if (Handle)
        {
            ret = Handle->Completed;
            free(Handle);
        }
        return ret;
    }

    /* Wait for the async RPC to complete (includes writer-side bulk push) */
    double t_rpc = mercury_wtime();
    hret = margo_wait(Handle->request);
    if (hret != HG_SUCCESS)
    {
        fprintf(stderr, "Mercury: margo_wait failed with error %d\n", hret);
        Handle->RS_Stream->TimeInRPC += mercury_wtime() - t_rpc;
        goto cleanup;
    }

    /* Get the RPC output — the writer already pushed data into our buffer */
    hret = margo_get_output(Handle->handle, &out);
    if (hret != HG_SUCCESS)
    {
        fprintf(stderr, "Mercury: margo_get_output failed with error %d\n", hret);
        goto cleanup;
    }

    Handle->RS_Stream->TimeInRPC += mercury_wtime() - t_rpc;
    Handle->RS_Stream->RPCCount++;

    if (out.ret == 0)
    {
        Handle->Completed = 1;
        Handle->RS_Stream->Stats->DataBytesReceived += Handle->Length;
    }

    margo_free_output(Handle->handle, &out);
    ret = Handle->Completed;

cleanup:
    Handle->AsyncPending = 0;
    if (Handle->local_bulk != HG_BULK_NULL)
    {
        margo_bulk_free(Handle->local_bulk);
    }
    if (Handle->handle != HG_HANDLE_NULL)
    {
        margo_destroy(Handle->handle);
        Handle->handle = HG_HANDLE_NULL;
    }
    free(Handle);

    return ret;
}

/*
 * NotifyConnFailure - Handle connection failure
 */
static void MercuryNotifyConnFailure(CP_Services Svcs, DP_RS_Stream RS_Stream_v, int FailedPeerRank)
{
    /* Mercury handles connection failures internally */
    /* We could mark the peer as failed and fail pending operations */
}

/*
 * ProvideTimestep - Register timestep data on writer side
 */
static void MercuryProvideTimestep(CP_Services Svcs, DP_WS_Stream WS_Stream_v,
                                   struct _SstData *Data, struct _SstData *LocalMetadata,
                                   size_t Timestep, void **TimestepInfoPtr)
{
    Mercury_WS_Stream WS_Stream = (Mercury_WS_Stream)WS_Stream_v;
    TimestepList Entry = malloc(sizeof(struct _TimestepEntry));

    Entry->Timestep = Timestep;
    Entry->Data = Data;
    Entry->DP_TimestepInfo = NULL;
    Entry->Next = WS_Stream->Timesteps;
    WS_Stream->Timesteps = Entry;

    *TimestepInfoPtr = NULL;
}

/*
 * ReleaseTimestep - Release timestep data on writer side
 */
static void MercuryReleaseTimestep(CP_Services Svcs, DP_WS_Stream WS_Stream_v, size_t Timestep)
{
    Mercury_WS_Stream WS_Stream = (Mercury_WS_Stream)WS_Stream_v;
    TimestepList List = WS_Stream->Timesteps;

    if (List && List->Timestep == Timestep)
    {
        WS_Stream->Timesteps = List->Next;
        if (List->DP_TimestepInfo)
            free(List->DP_TimestepInfo);
        free(List);
        return;
    }

    while (List != NULL && List->Next != NULL)
    {
        if (List->Next->Timestep == Timestep)
        {
            TimestepList tmp = List->Next;
            List->Next = tmp->Next;
            if (tmp->DP_TimestepInfo)
                free(tmp->DP_TimestepInfo);
            free(tmp);
            return;
        }
        List = List->Next;
    }

    fprintf(stderr, "Failed to release Timestep %zu, not found\n", Timestep);
}

/*
 * GetPriority - Return priority for this dataplane
 */
static int MercuryGetPriority(CP_Services Svcs, void *CP_Stream, struct _SstParams *Params)
{
    /* Check if Mercury is available and return appropriate priority */
    /* Higher priority means more preferred */
    /* Return -1 if Mercury cannot be used */

    /* For now, return a moderate priority if Mercury is compiled in */
    return 50; /* Medium priority */
}

/*
 * Contact Information Format Descriptions (for FFS)
 */
static FMField MercuryReaderContactList[] = {
    {"MercuryString", "string", sizeof(char *), FMOffset(MercuryReaderContactInfo, MercuryString)},
    {"reader_ID", "integer", sizeof(void *), FMOffset(MercuryReaderContactInfo, RS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MercuryReaderContactStructs[] = {
    {"MercuryReaderContactInfo", MercuryReaderContactList, sizeof(struct _MercuryReaderContactInfo),
     NULL},
    {NULL, NULL, 0, NULL}};

static FMField MercuryWriterContactList[] = {
    {"MercuryString", "string", sizeof(char *), FMOffset(MercuryWriterContactInfo, MercuryString)},
    {"writer_ID", "integer", sizeof(void *), FMOffset(MercuryWriterContactInfo, WS_Stream)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MercuryWriterContactStructs[] = {
    {"MercuryWriterContactInfo", MercuryWriterContactList, sizeof(struct _MercuryWriterContactInfo),
     NULL},
    {NULL, NULL, 0, NULL}};

/*
 * LoadMercuryDP - Return the Mercury DataPlane interface
 */
static struct _CP_DP_Interface mercuryDPInterface;

extern CP_DP_Interface LoadMercuryDP()
{
    memset(&mercuryDPInterface, 0, sizeof(mercuryDPInterface));

    mercuryDPInterface.DPName = "mercury";
    mercuryDPInterface.ReaderContactFormats = MercuryReaderContactStructs;
    mercuryDPInterface.WriterContactFormats = MercuryWriterContactStructs;
    mercuryDPInterface.TimestepInfoFormats = NULL;

    mercuryDPInterface.initReader = MercuryInitReader;
    mercuryDPInterface.initWriter = MercuryInitWriter;
    mercuryDPInterface.initWriterPerReader = MercuryInitWriterPerReader;
    mercuryDPInterface.provideWriterDataToReader = MercuryProvideWriterDataToReader;

    mercuryDPInterface.readRemoteMemory = (CP_DP_ReadRemoteMemoryFunc)MercuryReadRemoteMemory;
    mercuryDPInterface.waitForCompletion = MercuryWaitForCompletion;
    mercuryDPInterface.notifyConnFailure = MercuryNotifyConnFailure;

    mercuryDPInterface.provideTimestep = (CP_DP_ProvideTimestepFunc)MercuryProvideTimestep;
    mercuryDPInterface.releaseTimestep = (CP_DP_ReleaseTimestepFunc)MercuryReleaseTimestep;

    mercuryDPInterface.destroyReader = MercuryDestroyReader;
    mercuryDPInterface.destroyWriter = MercuryDestroyWriter;
    mercuryDPInterface.destroyWriterPerReader = MercuryDestroyWriterPerReader;

    mercuryDPInterface.getPriority = MercuryGetPriority;
    mercuryDPInterface.unGetPriority = NULL;

    return &mercuryDPInterface;
}
