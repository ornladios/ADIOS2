#include <assert.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "adios2/common/ADIOSConfig.h"
#include <atl.h>
#include <evpath.h>
#include <pthread.h>

#include "sst.h"

#include "adios2/toolkit/profiling/taustubs/taustubs.h"
#include "cp_internal.h"

static char *readContactInfoFile(const char *Name, SstStream Stream,
                                 int Timeout)
{
    size_t len = strlen(Name) + strlen(SST_POSTFIX) + 1;
    char *FileName = malloc(len);
    int Badfile = 0;
    int ZeroCount = 0;
    FILE *WriterInfo;
    int64_t TimeoutRemaining = Timeout * 1000 * 1000;
    int64_t WaitWarningRemaining = 5 * 1000 * 1000;
    long SleepInterval = 100000;
    snprintf(FileName, len, "%s" SST_POSTFIX, Name);
    CP_verbose(Stream,
               "Looking for writer contact in file %s, with timeout %d secs\n",
               FileName, Timeout);
redo:
    WriterInfo = fopen(FileName, "r");
    while (!WriterInfo)
    {
        // CMusleep(Stream->CPInfo->cm, SleepInterval);
        usleep(SleepInterval);
        TimeoutRemaining -= SleepInterval;
        WaitWarningRemaining -= SleepInterval;
        if (WaitWarningRemaining == 0)
        {
            fprintf(stderr,
                    "ADIOS2 SST Engine waiting for contact information "
                    "file %s to be created\n",
                    Name);
        }
        if (TimeoutRemaining <= 0)
        {
            return NULL;
        }
        WriterInfo = fopen(FileName, "r");
    }
    struct stat Buf;
    fstat(fileno(WriterInfo), &Buf);
    int Size = Buf.st_size;
    if (Size == 0)
    {
        //  Try again, it might look zero momentarily, but shouldn't stay that
        //  way.
        ZeroCount++;
        if (ZeroCount < 5)
        {
            // We'll give it several attempts (and some time) to go non-zero
            usleep(SleepInterval);
            goto redo;
        }
    }

    if (Size < strlen(SSTMAGICV0))
    {
        Badfile++;
    }
    else
    {
        char Tmp[strlen(SSTMAGICV0)];
        if (fread(Tmp, strlen(SSTMAGICV0), 1, WriterInfo) != 1)
        {
            fprintf(stderr,
                    "Filesystem read failed in SST Open, failing operation\n");
            fclose(WriterInfo);
            Badfile++;
        }
        Size -= strlen(SSTMAGICV0);
        if (strncmp(Tmp, SSTMAGICV0, strlen(SSTMAGICV0)) != 0)
        {
            Badfile++;
        }
    }
    if (Badfile)
    {
        fprintf(stderr,
                "!!! File %s is not an ADIOS2 SST Engine Contact file\n",
                FileName);
        free(FileName);
        fclose(WriterInfo);
        return NULL;
    }
    free(FileName);
    char *Buffer = calloc(1, Size + 1);
    if (fread(Buffer, Size, 1, WriterInfo) != 1)
    {
        fprintf(stderr,
                "Filesystem read failed in SST Open, failing operation\n");
        free(Buffer);
        fclose(WriterInfo);
        return NULL;
    }
    fclose(WriterInfo);
    return Buffer;
}

static char *readContactInfoScreen(const char *Name, SstStream Stream)
{
    char Input[10240];
    char *Skip = Input;
    fprintf(stdout,
            "Please enter the contact information associated with SST "
            "input stream \"%s\":\n",
            Name);
    if (fgets(Input, sizeof(Input), stdin) == NULL)
    {
        fprintf(stdout, "Read from stdin failed, exiting\n");
        exit(1);
    }
    while (isspace(*Skip))
        Skip++;
    return strdup(Skip);
}

static char *readContactInfo(const char *Name, SstStream Stream, int Timeout)
{
    switch (Stream->RegistrationMethod)
    {
    case SstRegisterFile:
        return readContactInfoFile(Name, Stream, Timeout);
        break;
    case SstRegisterScreen:
        return readContactInfoScreen(Name, Stream);
        break;
    case SstRegisterCloud:
        /* not yet */
        return NULL;
        break;
    }
}

extern void ReaderConnCloseHandler(CManager cm, CMConnection ClosedConn,
                                   void *client_data)
{
    TAU_START_FUNC();
    SstStream Stream = (SstStream)client_data;
    int FailedPeerRank = -1;
    CP_verbose(Stream, "Reader-side close handler invoked\n");
    for (int i = 0; i < Stream->WriterCohortSize; i++)
    {
        if (Stream->ConnectionsToWriter[i].CMconn == ClosedConn)
        {
            FailedPeerRank = i;
        }
    }

    if (Stream->Status == Established)
    {
        /*
         * tag our reader instance as failed.
         * If any instance is failed, we should remove all, but that requires a
         * global operation, so prep.
         */
        CP_verbose(Stream, "Reader-side Rank received a "
                           "connection-close event during normal "
                           "operations, peer likely failed\n");
        pthread_mutex_lock(&Stream->DataLock);
        Stream->Status = PeerFailed;
        pthread_cond_signal(&Stream->DataCondition);
        pthread_mutex_unlock(&Stream->DataLock);
        CP_verbose(
            Stream,
            "The close was for connection to writer peer %d, notifying DP\n",
            FailedPeerRank);
        /* notify DP of failure.  This should terminate any waits currently
         * pending in the DP for that rank */
        Stream->DP_Interface->notifyConnFailure(&Svcs, Stream->DP_Stream,
                                                FailedPeerRank);
    }
    else if ((Stream->Status == PeerClosed) || (Stream->Status == PeerFailed) ||
             (Stream->Status == Closed))
    {
        /* ignore this.  We expect a close after the connection is marked closed
         */
        CP_verbose(Stream, "Reader-side Rank received a "
                           "connection-close event after close, "
                           "not unexpected\n");
        Stream->DP_Interface->notifyConnFailure(&Svcs, Stream->DP_Stream,
                                                FailedPeerRank);
    }
    else
    {
        fprintf(stderr, "Got an unexpected connection close event\n");
        CP_verbose(Stream,
                   "Reader-side Rank received a "
                   "connection-close event in unexpected "
                   "status %s\n",
                   SSTStreamStatusStr[Stream->Status]);
    }
    TAU_STOP_FUNC();
}

extern long SstCurrentStep(SstStream Stream) { return Stream->ReaderTimestep; }

static void releasePriorTimesteps(SstStream Stream, long Latest);
static void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
                                    void **WS_StreamPtr);

static void **ParticipateInReaderInitDataExchange(SstStream Stream,
                                                  void *dpInfo,
                                                  void **ret_data_block)
{

    struct _CP_DP_PairInfo combined_init;
    struct _CP_ReaderInitInfo cpInfo;

    struct _CP_DP_PairInfo **pointers;

    cpInfo.ContactInfo = CP_GetContactString(Stream, NULL);
    cpInfo.ReaderID = Stream;

    combined_init.CP_Info = (void **)&cpInfo;
    combined_init.DP_Info = dpInfo;

    pointers = (struct _CP_DP_PairInfo **)CP_consolidateDataToRankZero(
        Stream, &combined_init, Stream->CPInfo->PerRankReaderInfoFormat,
        ret_data_block);
    free(cpInfo.ContactInfo);
    return (void **)pointers;
}

static int HasAllPeers(SstStream Stream)
{
    int i, StillWaiting = 0;
    if (!Stream->ConnectionsToWriter)
    {
        CP_verbose(Stream, "Waiting for first Peer notification\n");
        return 0;
    }
    i = 0;
    while (Stream->Peers[i] != -1)
    {
        int peer = Stream->Peers[i];
        if (Stream->ConnectionsToWriter[peer].CMconn == NULL)
            StillWaiting++;
        i++;
    }
    if (StillWaiting == 0)
    {
        CP_verbose(Stream, "Rank %d has all forward peer connections\n",
                   Stream->Rank);
        return 1;
    }
    else
    {
        CP_verbose(Stream, "Rank %d waiting for %d forward peer connections\n",
                   Stream->Rank, StillWaiting);
        return 0;
    }
}

attr_list ContactWriter(SstStream Stream, char *Filename, SstParams Params,
                        MPI_Comm comm, CMConnection *conn_p,
                        void **WriterFileID_p)
{
    int DataSize = 0;
    attr_list RetVal = NULL;

    if (Stream->Rank == 0)
    {
        char *Writer0Contact =
            readContactInfo(Filename, Stream, Params->OpenTimeoutSecs);
        char *CMContactString;
        CMConnection conn = NULL;
        attr_list WriterRank0Contact;

        if (Writer0Contact)
        {

            CMContactString =
                malloc(strlen(Writer0Contact)); /* at least long enough */
            sscanf(Writer0Contact, "%p:%s", WriterFileID_p, CMContactString);
            //        printf("Writer contact info is fileID %p, contact info
            //        %s\n",
            //               WriterFileID, CMContactString);
            free(Writer0Contact);

            if (globalNetinfoCallback)
            {
                (globalNetinfoCallback)(1, CP_GetContactString(Stream, NULL),
                                        IPDiagString);
                (globalNetinfoCallback)(2, CMContactString, NULL);
            }
            WriterRank0Contact = attr_list_from_string(CMContactString);
            conn = CMget_conn(Stream->CPInfo->cm, WriterRank0Contact);
            free_attr_list(WriterRank0Contact);
        }
        if (conn)
        {
            DataSize = strlen(CMContactString);
            *conn_p = conn;
        }
        else
        {
            DataSize = 0;
            *conn_p = NULL;
        }
        SMPI_Bcast(&DataSize, 1, MPI_INT, 0, Stream->mpiComm);
        if (DataSize != 0)
        {
            SMPI_Bcast(CMContactString, DataSize, MPI_CHAR, 0, Stream->mpiComm);
            RetVal = attr_list_from_string(CMContactString);
        }
    }
    else
    {
        SMPI_Bcast(&DataSize, 1, MPI_INT, 0, Stream->mpiComm);
        if (DataSize != 0)
        {
            char *Buffer = malloc(DataSize);
            SMPI_Bcast(Buffer, DataSize, MPI_CHAR, 0, Stream->mpiComm);
            RetVal = attr_list_from_string(Buffer);
            free(Buffer);
        }
    }
    return RetVal;
}

SstStream SstReaderOpen(const char *Name, SstParams Params, MPI_Comm comm)
{
    SstStream Stream;
    void *dpInfo;
    struct _CP_DP_PairInfo **pointers;
    void *data_block;
    void *free_block;
    writer_data_t ReturnData;
    struct _ReaderActivateMsg Msg;
    struct timeval Start, Stop, Diff;
    char *Filename = strdup(Name);
    CMConnection rank0_to_rank0_conn = NULL;
    void *WriterFileID;

    Stream = CP_newStream();
    Stream->Role = ReaderRole;
    Stream->mpiComm = comm;

    SMPI_Comm_rank(Stream->mpiComm, &Stream->Rank);
    SMPI_Comm_size(Stream->mpiComm, &Stream->CohortSize);

    CP_validateParams(Stream, Params, 0 /* reader */);
    Stream->ConfigParams = Params;

    Stream->DP_Interface = SelectDP(&Svcs, Stream, Stream->ConfigParams);

    Stream->CPInfo =
        CP_getCPInfo(Stream->DP_Interface, Stream->ConfigParams->ControlModule);

    Stream->FinalTimestep = INT_MAX; /* set this on close */
    Stream->LastDPNotifiedTimestep = -1;

    gettimeofday(&Start, NULL);

    attr_list WriterContactAttributes = ContactWriter(
        Stream, Filename, Params, comm, &rank0_to_rank0_conn, &WriterFileID);

    if (WriterContactAttributes == NULL)
        return NULL;

    Stream->DP_Stream = Stream->DP_Interface->initReader(
        &Svcs, Stream, &dpInfo, Stream->ConfigParams, WriterContactAttributes);

    pointers = (struct _CP_DP_PairInfo **)ParticipateInReaderInitDataExchange(
        Stream, dpInfo, &data_block);

    if (Stream->Rank == 0)
    {
        struct _CombinedWriterInfo WriterData;
        struct _ReaderRegisterMsg ReaderRegister;

        memset(&ReaderRegister, 0, sizeof(ReaderRegister));
        ReaderRegister.WriterFile = WriterFileID;
        ReaderRegister.WriterResponseCondition =
            CMCondition_get(Stream->CPInfo->cm, rank0_to_rank0_conn);
        ReaderRegister.ReaderCohortSize = Stream->CohortSize;
        switch (Stream->ConfigParams->SpeculativePreloadMode)
        {
        case SpecPreloadOff:
        case SpecPreloadOn:
            ReaderRegister.SpecPreload =
                Stream->ConfigParams->SpeculativePreloadMode;
            break;
        case SpecPreloadAuto:
            ReaderRegister.SpecPreload = SpecPreloadOff;
            if (Stream->CohortSize <=
                Stream->ConfigParams->SpecAutoNodeThreshold)
            {
                ReaderRegister.SpecPreload = SpecPreloadOn;
            }
            break;
        }

        ReaderRegister.CP_ReaderInfo =
            malloc(ReaderRegister.ReaderCohortSize * sizeof(void *));
        ReaderRegister.DP_ReaderInfo =
            malloc(ReaderRegister.ReaderCohortSize * sizeof(void *));
        for (int i = 0; i < ReaderRegister.ReaderCohortSize; i++)
        {
            ReaderRegister.CP_ReaderInfo[i] =
                (CP_ReaderInitInfo)pointers[i]->CP_Info;
            ReaderRegister.DP_ReaderInfo[i] = pointers[i]->DP_Info;
        }
        /* the response value is set in the handler */
        struct _WriterResponseMsg *response = NULL;
        CMCondition_set_client_data(Stream->CPInfo->cm,
                                    ReaderRegister.WriterResponseCondition,
                                    &response);

        if (CMwrite(rank0_to_rank0_conn, Stream->CPInfo->ReaderRegisterFormat,
                    &ReaderRegister) != 1)
        {
            CP_verbose(Stream,
                       "Message failed to send to writer in SstReaderOpen\n");
        }
        free(ReaderRegister.CP_ReaderInfo);
        free(ReaderRegister.DP_ReaderInfo);

        /* wait for "go" from writer */
        CP_verbose(
            Stream,
            "Waiting for writer response message in SstReadOpen(\"%s\")\n",
            Filename, ReaderRegister.WriterResponseCondition);
        CMCondition_wait(Stream->CPInfo->cm,
                         ReaderRegister.WriterResponseCondition);
        CP_verbose(Stream,
                   "finished wait writer response message in read_open\n");

        if (response)
        {
            WriterData.WriterCohortSize = response->WriterCohortSize;
            WriterData.WriterConfigParams = response->WriterConfigParams;
            WriterData.StartingStepNumber = response->NextStepNumber;
            WriterData.CP_WriterInfo = response->CP_WriterInfo;
            WriterData.DP_WriterInfo = response->DP_WriterInfo;
        }
        ReturnData = CP_distributeDataFromRankZero(
            Stream, &WriterData, Stream->CPInfo->CombinedWriterInfoFormat,
            &free_block);
    }
    else
    {
        ReturnData = CP_distributeDataFromRankZero(
            Stream, NULL, Stream->CPInfo->CombinedWriterInfoFormat,
            &free_block);
    }

    free(data_block);

    if (ReturnData->WriterCohortSize == -1)
    {
        /* Rank 0 found no writer at that contact point, fail the stream */
        free(free_block);
        return NULL;
    }

    if (Stream->Rank == 0)
    {
        CP_verbose(Stream,
                   "Opening Reader Stream.\nWriter stream params are:\n");
        CP_dumpParams(Stream, ReturnData->WriterConfigParams,
                      0 /* writer side */);
        CP_verbose(Stream, "Reader stream params are:\n");
        CP_dumpParams(Stream, Stream->ConfigParams, 1 /* reader side */);
    }

    //    printf("I am reader rank %d, my info on writers is:\n", Stream->Rank);
    //    FMdump_data(FMFormat_of_original(Stream->CPInfo->combined_writer_Format),
    //                ReturnData, 1024000);
    //    printf("\n");

    Stream->WriterCohortSize = ReturnData->WriterCohortSize;
    Stream->WriterConfigParams = ReturnData->WriterConfigParams;
    if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
    {
        CP_verbose(Stream, "Writer is doing FFS-based marshalling\n");
    }
    if (Stream->WriterConfigParams->MarshalMethod == SstMarshalBP)
    {
        CP_verbose(Stream, "Writer is doing BP-based marshalling\n");
    }
    if (Stream->WriterConfigParams->CPCommPattern == SstCPCommMin)
    {
        CP_verbose(
            Stream,
            "Writer is using Minimum Connection Communication pattern (min)\n");
    }
    if (Stream->WriterConfigParams->CPCommPattern == SstCPCommPeer)
    {
        CP_verbose(Stream,
                   "Writer is using Peer-based Communication pattern (peer)\n");
    }
    Stream->ReaderTimestep = ReturnData->StartingStepNumber - 1;

    if (Stream->WriterConfigParams->CPCommPattern == SstCPCommPeer)
    {
        /*
         *  Wait for connections and messages from writer side peers
         */
        getPeerArrays(Stream->CohortSize, Stream->Rank,
                      Stream->WriterCohortSize, &Stream->Peers, NULL);

        pthread_mutex_lock(&Stream->DataLock);
        while (!HasAllPeers(Stream))
        {
            /* wait until we get the timestep metadata or something else changes
             */
            pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
        }
        pthread_mutex_unlock(&Stream->DataLock);
    }
    else
    {
        if (!Stream->ConnectionsToWriter)
        {
            Stream->ConnectionsToWriter =
                calloc(sizeof(CP_PeerConnection), ReturnData->WriterCohortSize);
        }
    }

    for (int i = 0; i < ReturnData->WriterCohortSize; i++)
    {
        attr_list attrs =
            attr_list_from_string(ReturnData->CP_WriterInfo[i]->ContactInfo);
        Stream->ConnectionsToWriter[i].ContactList = attrs;
        Stream->ConnectionsToWriter[i].RemoteStreamID =
            ReturnData->CP_WriterInfo[i]->WriterID;
    }

    // Deref the original connection to writer rank 0 (might still be open as a
    // peer)
    if (Stream->WriterConfigParams->CPCommPattern == SstCPCommPeer)
    {
        if (rank0_to_rank0_conn)
        {
            CMConnection_dereference(rank0_to_rank0_conn);
        }
    }
    else
    {
        /* only rely on the rank 0 to rank 0 that we already have (if we're rank
         * 0) */
        if (rank0_to_rank0_conn)
        {
            CMConnection conn = rank0_to_rank0_conn;
            Stream->ConnectionsToWriter[0].CMconn = conn;
            CMConnection_add_reference(conn);
            CMconn_register_close_handler(conn, ReaderConnCloseHandler,
                                          (void *)Stream);
        }
    }
    Stream->DP_Interface->provideWriterDataToReader(
        &Svcs, Stream->DP_Stream, ReturnData->WriterCohortSize,
        Stream->ConnectionsToWriter, ReturnData->DP_WriterInfo);
    pthread_mutex_lock(&Stream->DataLock);
    Stream->Status = Established;
    pthread_mutex_unlock(&Stream->DataLock);
    CP_verbose(Stream, "Sending Reader Activate messages to writer\n");
    memset(&Msg, 0, sizeof(Msg));
    sendOneToEachWriterRank(Stream, Stream->CPInfo->ReaderActivateFormat, &Msg,
                            &Msg.WSR_Stream);
    CP_verbose(Stream,
               "Finish opening Stream \"%s\", starting with Step number %d\n",
               Filename, ReturnData->StartingStepNumber);
    gettimeofday(&Stop, NULL);
    timersub(&Stop, &Start, &Diff);
    Stream->OpenTimeSecs = (double)Diff.tv_usec / 1e6 + Diff.tv_sec;
    gettimeofday(&Stream->ValidStartTime, NULL);
    Stream->Filename = Filename;
    Stream->ParamsBlock = free_block;
    AddToLastCallFreeList(Stream);

    return Stream;
}

extern void SstReaderGetParams(SstStream Stream,
                               SstMarshalMethod *WriterMarshalMethod)
{
    *WriterMarshalMethod = Stream->WriterConfigParams->MarshalMethod;
}

/*
 * CP_PeerSetupHandler receives incoming PeerSetup messages to setup the
 * reader-side Peer list
 */
extern void CP_PeerSetupHandler(CManager cm, CMConnection conn, void *Msg_v,
                                void *client_data, attr_list attrs)
{
    TAU_START_FUNC();
    SstStream Stream;
    struct _PeerSetupMsg *Msg = (struct _PeerSetupMsg *)Msg_v;
    Stream = (SstStream)Msg->RS_Stream;
    pthread_mutex_lock(&Stream->DataLock);
    if (!Stream->ConnectionsToWriter)
    {
        Stream->ConnectionsToWriter =
            calloc(sizeof(CP_PeerConnection), Msg->WriterCohortSize);
    }
    CP_verbose(Stream, "Received peer setup from rank %d, conn %p\n",
               Msg->WriterRank, conn);
    if (Msg->WriterRank != -1)
    {
        Stream->ConnectionsToWriter[Msg->WriterRank].CMconn = conn;
        CMConnection_add_reference(conn);
    }
    CMconn_register_close_handler(conn, ReaderConnCloseHandler, (void *)Stream);
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
    TAU_STOP_FUNC();
}

void queueTimestepMetadataMsgAndNotify(SstStream Stream,
                                       struct _TimestepMetadataMsg *tsm,
                                       CMConnection conn)
{
    if (tsm->Timestep < Stream->DiscardPriorTimestep)
    {
        struct _ReleaseTimestepMsg Msg;
        memset(&Msg, 0, sizeof(Msg));
        Msg.Timestep = tsm->Timestep;

        pthread_mutex_lock(&Stream->DataLock);
        /*
         * before discarding, install any precious metadata from this message
         */
        if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
        {
            FFSMarshalInstallPreciousMetadata(Stream, tsm);
        }
        pthread_mutex_unlock(&Stream->DataLock);

        /*
         * send each writer rank a release for this timestep (actually goes to
         * WSR
         * Streams)
         */
        CP_verbose(Stream,
                   "Sending ReleaseTimestep message for PRIOR DISCARD "
                   "timestep %d, one to each writer\n",
                   tsm->Timestep);
        if (tsm->Metadata != NULL)
        {
            CP_verbose(Stream,
                       "Sending ReleaseTimestep message for PRIOR DISCARD "
                       "timestep %d, one to each writer\n",
                       tsm->Timestep);
            sendOneToEachWriterRank(Stream,
                                    Stream->CPInfo->ReleaseTimestepFormat, &Msg,
                                    &Msg.WSR_Stream);
        }
        else
        {
            CP_verbose(Stream,
                       "Received discard notice for timestep %d, "
                       "ignoring in PRIOR DISCARD\n",
                       tsm->Timestep);
        }
        CMreturn_buffer(Stream->CPInfo->cm, tsm);
        return;
    }

    pthread_mutex_lock(&Stream->DataLock);
    struct _TimestepMetadataList *New = malloc(sizeof(struct _RequestQueue));
    New->MetadataMsg = tsm;
    New->Next = NULL;
    if (Stream->Timesteps)
    {
        struct _TimestepMetadataList *Last = Stream->Timesteps;
        while (Last->Next)
        {
            Last = Last->Next;
        }
        Last->Next = New;
    }
    else
    {
        Stream->Timesteps = New;
    }
    CP_verbose(Stream,
               "Received a Timestep metadata message for timestep %d, "
               "signaling condition\n",
               tsm->Timestep);

    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
    if ((Stream->WriterConfigParams->CPCommPattern == SstCPCommMin) &&
        (Stream->ConfigParams->AlwaysProvideLatestTimestep))
    {
        /*
         * IFF we are in CommMin mode, AND we are to always provide
         * the newest timestep, then when a new timestep arrives then
         * we want to release timesteps that are older than it, NOT
         * INCLUDING ANY TIMESTEP IN CURRENT USE.
         */
        CP_verbose(Stream,
                   "Got a new timestep in AlwaysProvideLatestTimestep mode, "
                   "discard older than %d\n",
                   tsm->Timestep);
        releasePriorTimesteps(Stream, tsm->Timestep);
    }
}

void CP_TimestepMetadataHandler(CManager cm, CMConnection conn, void *Msg_v,
                                void *client_data, attr_list attrs)
{
    TAU_START_FUNC();
    SstStream Stream;
    struct _TimestepMetadataMsg *Msg = (struct _TimestepMetadataMsg *)Msg_v;
    Stream = (SstStream)Msg->RS_Stream;
    if (Stream->WriterConfigParams->CPCommPattern == SstCPCommPeer)
    {
        /* everyone is getting this */
        if (Msg->Metadata == NULL)
        {
            CP_verbose(
                Stream,
                "Received a message that timestep %d has been discarded\n",
                Msg->Timestep);

            pthread_mutex_lock(&Stream->DataLock);
            /*
             * before discarding, install any precious metadata from this
             * message
             */
            if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
            {
                FFSMarshalInstallPreciousMetadata(Stream, Msg);
            }
            pthread_mutex_unlock(&Stream->DataLock);

            return;
        }
        else
        {
            CP_verbose(
                Stream,
                "Received an incoming metadata message for timestep %d\n",
                Msg->Timestep);
        }
        /* arrange for this message data to stay around */
        CMtake_buffer(cm, Msg);

        queueTimestepMetadataMsgAndNotify(Stream, Msg, conn);
    }
    else
    {
        /* I must be rank 0 and only I got this, I'll need to distribute it to
         * everyone */
        /* arrange for this message data to stay around */
        CMtake_buffer(cm, Msg);

        queueTimestepMetadataMsgAndNotify(Stream, Msg, conn);
    }
    TAU_STOP_FUNC();
}

void CP_WriterResponseHandler(CManager cm, CMConnection conn, void *Msg_v,
                              void *client_data, attr_list attrs)
{
    TAU_REGISTER_THREAD();
    TAU_START_FUNC();
    struct _WriterResponseMsg *Msg = (struct _WriterResponseMsg *)Msg_v;
    struct _WriterResponseMsg **response_ptr;
    //    fprintf(stderr, "Received a writer_response message for condition
    //    %d\n",
    //            Msg->WriterResponseCondition);
    //    fprintf(stderr, "The responding writer has cohort of size %d :\n",
    //            Msg->writer_CohortSize);
    //    for (int i = 0; i < Msg->writer_CohortSize; i++) {
    //        fprintf(stderr, " rank %d CP contact info: %s, %p\n", i,
    //                Msg->CP_WriterInfo[i]->ContactInfo,
    //                Msg->CP_WriterInfo[i]->WriterID);
    //    }

    /* arrange for this message data to stay around */
    CMtake_buffer(cm, Msg);

    /* attach the message to the CMCondition so it an be retrieved by the main
     * thread */
    response_ptr =
        CMCondition_get_client_data(cm, Msg->WriterResponseCondition);
    *response_ptr = Msg;

    /* wake the main thread */
    CMCondition_signal(cm, Msg->WriterResponseCondition);
    TAU_STOP_FUNC();
}

extern void CP_WriterCloseHandler(CManager cm, CMConnection conn, void *Msg_v,
                                  void *client_data, attr_list attrs)
{
    TAU_START_FUNC();
    WriterCloseMsg Msg = (WriterCloseMsg)Msg_v;
    SstStream Stream = (SstStream)Msg->RS_Stream;

    CP_verbose(Stream,
               "Received a writer close message. "
               "Timestep %d was the final timestep.\n",
               Msg->FinalTimestep);

    pthread_mutex_lock(&Stream->DataLock);
    Stream->FinalTimestep = Msg->FinalTimestep;
    Stream->Status = PeerClosed;
    /* wake anyone that might be waiting */
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
    TAU_STOP_FUNC();
}

extern void CP_CommPatternLockedHandler(CManager cm, CMConnection conn,
                                        void *Msg_v, void *client_data,
                                        attr_list attrs)
{
    CommPatternLockedMsg Msg = (CommPatternLockedMsg)Msg_v;
    SstStream Stream = (SstStream)Msg->RS_Stream;

    CP_verbose(
        Stream,
        "Received a CommPatternLocked message, beginning with Timestep %d.\n",
        Msg->Timestep);

    pthread_mutex_lock(&Stream->DataLock);
    Stream->CommPatternLocked = 1;
    Stream->CommPatternLockedTimestep = Msg->Timestep;
    pthread_mutex_unlock(&Stream->DataLock);
}

static long MaxQueuedMetadata(SstStream Stream)
{
    struct _TimestepMetadataList *Next;
    long MaxTimestep = -1;
    pthread_mutex_lock(&Stream->DataLock);
    Next = Stream->Timesteps;
    if (Next == NULL)
    {
        CP_verbose(Stream, "MaxQueued Timestep returning -1\n");
        pthread_mutex_unlock(&Stream->DataLock);
        return -1;
    }
    while (Next)
    {
        if (Next->MetadataMsg->Timestep >= MaxTimestep)
        {
            MaxTimestep = Next->MetadataMsg->Timestep;
        }
        Next = Next->Next;
    }
    pthread_mutex_unlock(&Stream->DataLock);
    CP_verbose(Stream, "MaxQueued Timestep returning %ld\n", MaxTimestep);
    return MaxTimestep;
}

static long NextQueuedMetadata(SstStream Stream)
{
    struct _TimestepMetadataList *Next;
    long MinTimestep = LONG_MAX;
    pthread_mutex_lock(&Stream->DataLock);
    Next = Stream->Timesteps;
    if (Next == NULL)
    {
        CP_verbose(Stream, "NextQueued Timestep returning -1\n");
        pthread_mutex_unlock(&Stream->DataLock);
        return -1;
    }
    while (Next)
    {
        if (Next->MetadataMsg->Timestep <= MinTimestep)
        {
            MinTimestep = Next->MetadataMsg->Timestep;
        }
        Next = Next->Next;
    }
    pthread_mutex_unlock(&Stream->DataLock);
    CP_verbose(Stream, "NextQueued Timestep returning %ld\n", MinTimestep);
    return MinTimestep;
}

static void triggerDataCondition(CManager cm, void *vStream)
{
    SstStream Stream = (SstStream)vStream;

    pthread_mutex_lock(&Stream->DataLock);
    /* wake the sleeping main thread for timeout */
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
}

static void waitForMetadataWithTimeout(SstStream Stream, float timeout_secs)
{
    struct _TimestepMetadataList *Next;
    struct timeval start, now, end;
    int timeout_int_sec = floor(timeout_secs);
    int timeout_int_usec = ((timeout_secs - floorf(timeout_secs)) * 1000000);
    CMTaskHandle TimeoutTask = NULL;

    pthread_mutex_lock(&Stream->DataLock);
    gettimeofday(&start, NULL);
    Next = Stream->Timesteps;
    CP_verbose(
        Stream,
        "Wait for metadata with timeout %g secs starting at time %ld.%06ld \n",
        timeout_secs, start.tv_sec, start.tv_usec);
    if (Next)
    {
        pthread_mutex_unlock(&Stream->DataLock);
        CP_verbose(Stream, "Returning from wait with timeout, NO TIMEOUT\n");
    }
    end.tv_sec = start.tv_sec + timeout_int_sec;
    end.tv_usec = start.tv_usec + timeout_int_usec;
    if (end.tv_usec > 1000000)
    {
        end.tv_sec++;
        end.tv_usec -= 1000000;
    }
    if (end.tv_sec < start.tv_sec)
    {
        // rollover
        end.tv_sec = INT_MAX;
    }
    // special case
    if (timeout_secs == 0.0)
    {
        pthread_mutex_unlock(&Stream->DataLock);
        CP_verbose(
            Stream,
            "Returning from wait With no data after zero timeout poll\n");
        return;
    }

    TimeoutTask =
        CMadd_delayed_task(Stream->CPInfo->cm, timeout_int_sec,
                           timeout_int_usec, triggerDataCondition, Stream);
    while (1)
    {
        Next = Stream->Timesteps;
        if (Next)
        {
            CMremove_task(TimeoutTask);
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream,
                       "Returning from wait with timeout, NO TIMEOUT\n");
            return;
        }
        if (Stream->Status != Established)
        {
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream, "Returning from wait with timeout, STREAM NO "
                               "LONGER ESTABLISHED\n");
            return;
        }
        gettimeofday(&now, NULL);
        CP_verbose(Stream, "timercmp, now is %ld.%06ld    end is %ld.%06ld \n",
                   now.tv_sec, now.tv_usec, end.tv_sec, end.tv_usec);
        if (timercmp(&now, &end, >))
        {
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream, "Returning from wait after timing out\n");
            return;
        }
        /* wait until we get the timestep metadata or something else changes */
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    /* NOTREACHED */
}

static void releasePriorTimesteps(SstStream Stream, long Latest)
{
    struct _TimestepMetadataList *Next, *Last;
    TSMetadataList FoundTS = NULL;
    pthread_mutex_lock(&Stream->DataLock);
    CP_verbose(Stream, "Releasing any timestep earlier than %d\n", Latest);
    Next = Stream->Timesteps;
    Last = NULL;
    while (Next)
    {
        if ((Next->MetadataMsg->Timestep < Latest) &&
            (Next->MetadataMsg->Timestep != Stream->CurrentWorkingTimestep))
        {
            struct _TimestepMetadataList *This = Next;
            struct _ReleaseTimestepMsg Msg;
            Next = This->Next;

            /*
             * before discarding, install any precious metadata from this
             * message
             */
            if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
            {
                FFSMarshalInstallPreciousMetadata(Stream, This->MetadataMsg);
            }

            memset(&Msg, 0, sizeof(Msg));
            Msg.Timestep = This->MetadataMsg->Timestep;

            /*
             * send each writer rank a release for this timestep (actually goes
             * to WSR
             * Streams)
             */
            CP_verbose(Stream,
                       "Sending ReleaseTimestep message for RELEASE "
                       "PRIOR timestep %d, one to each writer\n",
                       This->MetadataMsg->Timestep);
            sendOneToEachWriterRank(Stream,
                                    Stream->CPInfo->ReleaseTimestepFormat, &Msg,
                                    &Msg.WSR_Stream);
            CMreturn_buffer(Stream->CPInfo->cm, This->MetadataMsg);
            if (Last == NULL)
            {
                Stream->Timesteps = Next;
            }
            else
            {
                Last->Next = Next;
            }
            free(This);
        }
        else
        {
            Last = Next;
            Next = Next->Next;
        }
    }
    pthread_mutex_unlock(&Stream->DataLock);
}

static void FreeTimestep(SstStream Stream, long Timestep)
{
    /*
     * remove local metadata for that timestep
     */
    struct _TimestepMetadataList *List = Stream->Timesteps;

    if (Stream->Timesteps->MetadataMsg->Timestep == Timestep)
    {
        Stream->Timesteps = List->Next;
        CMreturn_buffer(Stream->CPInfo->cm, List->MetadataMsg);
        free(List);
    }
    else
    {
        struct _TimestepMetadataList *last = List;
        List = List->Next;
        while (List != NULL)
        {
            if (List->MetadataMsg->Timestep == Timestep)
            {
                last->Next = List->Next;
                CMreturn_buffer(Stream->CPInfo->cm, List->MetadataMsg);
                free(List);
                break;
            }
            last = List;
            List = List->Next;
        }
    }
}

static TSMetadataList waitForNextMetadata(SstStream Stream, long LastTimestep)
{
    TSMetadataList FoundTS = NULL;
    pthread_mutex_lock(&Stream->DataLock);
    CP_verbose(Stream, "Wait for next metadata after last timestep %d\n",
               LastTimestep);
    while (1)
    {
        struct _TimestepMetadataList *Next;
        Next = Stream->Timesteps;
        while (Next)
        {
            CP_verbose(Stream, "Examining metadata for Timestep %d\n",
                       Next->MetadataMsg->Timestep);
            if ((Next->MetadataMsg->Metadata == NULL) && (FoundTS == NULL))
            {
                /*
                 * This is a dummy timestep for something that was
                 * discarded on the writer side.  Now is the time to
                 * install the 'precious' info that it carried
                 * (Attributes and formats) and then discard it.
                 */
                CP_verbose(Stream,
                           "SstAdvanceStep installing precious "
                           "metadata for discarded TS %d\n",
                           Next->MetadataMsg->Timestep);
                FFSMarshalInstallPreciousMetadata(Stream, Next->MetadataMsg);
                TSMetadataList Tmp = Next;
                Next = Next->Next;
                FreeTimestep(Stream, Tmp->MetadataMsg->Timestep);
                continue;
            }
            if (Next->MetadataMsg->Timestep >= LastTimestep)
            {
                if ((FoundTS == NULL) &&
                    (Next->MetadataMsg->Timestep > LastTimestep))
                {
                    FoundTS = Next;
                    break;
                }
                else if ((FoundTS != NULL) && (FoundTS->MetadataMsg->Timestep >
                                               Next->MetadataMsg->Timestep))
                {
                    FoundTS = Next;
                    break;
                }
            }
            Next = Next->Next;
        }
        if (FoundTS)
        {
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream, "Returning metadata for Timestep %d\n",
                       FoundTS->MetadataMsg->Timestep);
            Stream->CurrentWorkingTimestep = FoundTS->MetadataMsg->Timestep;
            return FoundTS;
        }
        /* didn't find a good next timestep, check Stream status */
        if ((Stream->Status != Established) ||
            ((Stream->FinalTimestep != INT_MAX) &&
             (Stream->FinalTimestep >= LastTimestep)))
        {
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream,
                       "Stream Final Timestep is %d, last timestep was %d\n",
                       Stream->FinalTimestep, LastTimestep);
            if (Stream->Status == NotOpen)
            {
                CP_verbose(Stream,
                           "Wait for next metadata returning NULL because "
                           "channel was never fully established\n");
            }
            else if (Stream->Status == PeerFailed)
            {
                CP_verbose(Stream,
                           "Wait for next metadata returning NULL because "
                           "the connection failed before final timestep "
                           "notification\n");
            }
            else
            {
                CP_verbose(Stream,
                           "Wait for next metadata returning NULL, status %d ",
                           Stream->Status);
            }
            /* closed or failed, return NULL */
            Stream->CurrentWorkingTimestep = -1;
            return NULL;
        }
        CP_verbose(Stream,
                   "Waiting for metadata for a Timestep later than TS %d\n",
                   LastTimestep);
        CP_verbose(Stream, "(PID %x) Stream status is %s\n", getpid(),
                   SSTStreamStatusStr[Stream->Status]);
        /* wait until we get the timestep metadata or something else changes */
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    /* NOTREACHED */
    pthread_mutex_unlock(&Stream->DataLock);
}

extern SstFullMetadata SstGetCurMetadata(SstStream Stream)
{
    return Stream->CurrentMetadata;
}

extern void *SstReadRemoteMemory(SstStream Stream, int Rank, long Timestep,
                                 size_t Offset, size_t Length, void *Buffer,
                                 void *DP_TimestepInfo)
{
    if (Stream->Stats)
        Stream->Stats->BytesTransferred += Length;
    return Stream->DP_Interface->readRemoteMemory(
        &Svcs, Stream->DP_Stream, Rank, Timestep, Offset, Length, Buffer,
        DP_TimestepInfo);
}

static void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
                                    void **WS_StreamPtr)
{
    if (s->WriterConfigParams->CPCommPattern == SstCPCommPeer)
    {
        int i = 0;
        while (s->Peers[i] != -1)
        {
            int peer = s->Peers[i];
            CMConnection conn = s->ConnectionsToWriter[peer].CMconn;
            /* add the writer Stream identifier to each outgoing
             * message */
            *WS_StreamPtr = s->ConnectionsToWriter[peer].RemoteStreamID;
            if (CMwrite(conn, f, Msg) != 1)
            {
                CP_verbose(s, "Message failed to send to writer %d (%p)\n",
                           peer, *WS_StreamPtr);
            }
            i++;
        }
    }
    else
    {
        if (s->Rank == 0)
        {
            int peer = 0;
            CMConnection conn = s->ConnectionsToWriter[peer].CMconn;
            /* add the writer Stream identifier to each outgoing
             * message */
            *WS_StreamPtr = s->ConnectionsToWriter[peer].RemoteStreamID;
            if (CMwrite(conn, f, Msg) != 1)
            {
                CP_verbose(s, "Message failed to send to writer %d (%p)\n",
                           peer, *WS_StreamPtr);
            }
        }
    }
}

extern void SstReaderDefinitionLock(SstStream Stream, long EffectiveTimestep)
{
    long Timestep = Stream->ReaderTimestep;
    struct _LockReaderDefinitionsMsg Msg;

    memset(&Msg, 0, sizeof(Msg));
    Msg.Timestep = EffectiveTimestep;

    sendOneToEachWriterRank(Stream, Stream->CPInfo->LockReaderDefinitionsFormat,
                            &Msg, &Msg.WSR_Stream);
}

extern void SstReleaseStep(SstStream Stream)
{
    long Timestep = Stream->ReaderTimestep;
    struct _ReleaseTimestepMsg Msg;

    TAU_START_FUNC();
    if ((Stream->WriterConfigParams->CPCommPattern == SstCPCommPeer) ||
        (Stream->Rank == 0))
    {
        pthread_mutex_lock(&Stream->DataLock);
        FreeTimestep(Stream, Timestep);
        pthread_mutex_unlock(&Stream->DataLock);
    }

    SMPI_Barrier(Stream->mpiComm);

    memset(&Msg, 0, sizeof(Msg));
    Msg.Timestep = Timestep;

    /*
     * send each writer rank a release for this timestep (actually goes to WSR
     * Streams)
     */
    CP_verbose(
        Stream,
        "Sending ReleaseTimestep message for timestep %d, one to each writer\n",
        Timestep);
    sendOneToEachWriterRank(Stream, Stream->CPInfo->ReleaseTimestepFormat, &Msg,
                            &Msg.WSR_Stream);

    if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
    {
        FFSClearTimestepData(Stream);
    }
    TAU_STOP_FUNC();
}

static void NotifyDPArrivedMetadataPeer(SstStream Stream)
{
    struct _TimestepMetadataList *TS;
    TS = Stream->Timesteps;
    while (TS)
    {
        if ((TS->MetadataMsg->Metadata != NULL) &&
            (TS->MetadataMsg->Timestep > Stream->LastDPNotifiedTimestep))
        {
            Stream->DP_Interface->timestepArrived(&Svcs, Stream->DP_Stream,
                                                  TS->MetadataMsg->Timestep,
                                                  TS->MetadataMsg->PreloadMode);
            Stream->LastDPNotifiedTimestep = TS->MetadataMsg->Timestep;
        }
        TS = TS->Next;
    }
}

/*
 * wait for metadata for Timestep indicated to arrive, or fail with EndOfStream
 * or Error
 */
static SstStatusValue SstAdvanceStepPeer(SstStream Stream, SstStepMode mode,
                                         const float timeout_sec)
{

    TSMetadataList Entry;

    TAU_START("Waiting on metadata per rank per timestep");

    if ((timeout_sec >= 0.0) || (mode == SstLatestAvailable))
    {
        struct _GlobalOpInfo
        {
            float timeout_sec;
            int mode;
            long LatestTimestep;
        };
        struct _GlobalOpInfo my_info;
        struct _GlobalOpInfo *global_info;
        long NextTimestep;

        if (Stream->Rank == 0)
        {
            global_info = malloc(sizeof(my_info) * Stream->CohortSize);
            CP_verbose(Stream,
                       "In special case of advancestep, mode is %d, "
                       "Timeout Sec is %g, flt_max is %g\n",
                       mode, timeout_sec, FLT_MAX);
        }
        my_info.LatestTimestep = MaxQueuedMetadata(Stream);
        my_info.timeout_sec = timeout_sec;
        my_info.mode = mode;
        SMPI_Gather(&my_info, sizeof(my_info), MPI_CHAR, global_info,
                    sizeof(my_info), MPI_CHAR, 0, Stream->mpiComm);
        if (Stream->Rank == 0)
        {
            long Biggest = -1;
            long Smallest = LONG_MAX;
            for (int i = 0; i < Stream->CohortSize; i++)
            {
                if (global_info[i].LatestTimestep > Biggest)
                {
                    Biggest = global_info[i].LatestTimestep;
                }
                if (global_info[i].LatestTimestep < Smallest)
                {
                    Smallest = global_info[i].LatestTimestep;
                }
            }

            /*
             * Several situations are possible here, depending upon
             * whether or not a timeout is specified and/or
             * LatestAvailable is specified, and whether or not we
             * have timesteps queued anywhere.  If they want
             * LatestAvailable and we have any Timesteps queued
             * anywhere, we decide upon a timestep to return and
             * assume that all ranks will get it soon (or else we're
             * in failure mode).  If there are no timesteps queued
             * anywhere, then we're going to wait for timeout seconds
             * ON RANK 0.  RANK 0 AND ONLY RANK 0 WILL DECIDE IF WE
             * TIMEOUT OR RETURN WITH DATA.  It is possible that other
             * ranks get timestep metadata before the timeout expires,
             * but we don't care.  Whatever would happen on rank 0 is
             * what happens everywhere.
             */

            if (Biggest == -1)
            {
                // AllQueuesEmpty
                if (timeout_sec >= 0.0)
                {
                    waitForMetadataWithTimeout(Stream, timeout_sec);
                }
                else
                {
                    waitForMetadataWithTimeout(Stream, FLT_MAX);
                }
                NextTimestep =
                    MaxQueuedMetadata(Stream); /* might be -1 if we timed out */
            }
            else
            {
                /*
                 * we've actually got a choice here.  "Smallest" is
                 * the LatestTimestep that everyone has.  "Biggest" is
                 * the Latest that someone has seen, and presumably
                 * others will see shortly.  I'm going to go with Biggest
                 * until I have a reason to prefer one or the other.
                 */
                if (mode == SstLatestAvailable)
                {
                    // latest available
                    CP_verbose(Stream,
                               "Returning Biggest timestep available "
                               "%ld because LatestAvailable "
                               "specified\n",
                               Biggest);
                    NextTimestep = Biggest;
                }
                else
                {
                    // next available (take the oldest that everyone has)
                    CP_verbose(Stream,
                               "Returning Smallest timestep available "
                               "%ld because NextAvailable specified\n",
                               Smallest);
                    NextTimestep = Smallest;
                }
            }
            if ((NextTimestep == -1) && (Stream->Status == PeerClosed))
            {
                /* force everyone to close */
                NextTimestep = -2;
            }
            if ((NextTimestep == -1) && (Stream->Status == PeerFailed))
            {
                /* force everyone to return failed */
                NextTimestep = -3;
            }
            SMPI_Bcast(&NextTimestep, 1, MPI_LONG, 0, Stream->mpiComm);
        }
        else
        {
            SMPI_Bcast(&NextTimestep, 1, MPI_LONG, 0, Stream->mpiComm);
        }
        if (NextTimestep == -2)
        {
            /* there was a peerClosed setting on rank0, we'll close */
            Stream->Status = PeerClosed;
            CP_verbose(Stream,
                       "SstAdvanceStep returning EndOfStream at timestep %d\n",
                       Stream->ReaderTimestep);
            return SstEndOfStream;
        }
        if (NextTimestep == -3)
        {
            /* there was a peerFailed setting on rank0, we'll fail */
            Stream->Status = PeerFailed;
            CP_verbose(Stream,
                       "SstAdvanceStep returning EndOfStream at timestep %d\n",
                       Stream->ReaderTimestep);
            return SstFatalError;
        }
        if (NextTimestep == -1)
        {
            CP_verbose(Stream, "AdvancestepPeer timing out on no data\n");
            return SstTimeout;
        }
        if (mode == SstLatestAvailable)
        {
            // latest available
            /* release all timesteps from before NextTimestep, then fall
             * through below */
            /* Side note: It is possible that someone could get a "prior"
             * timestep after this point.  It has to be released upon
             * arrival */
            CP_verbose(Stream,
                       "timed or Latest timestep, determined NextTimestep %d\n",
                       NextTimestep);
            Stream->DiscardPriorTimestep = NextTimestep;
            releasePriorTimesteps(Stream, NextTimestep);
        }
    }

    Entry = waitForNextMetadata(Stream, Stream->ReaderTimestep);

    TAU_STOP("Waiting on metadata per rank per timestep");

    NotifyDPArrivedMetadataPeer(Stream);
    if (Entry)
    {
        if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
        {
            TAU_START("FFS marshaling case");
            FFSMarshalInstallMetadata(Stream, Entry->MetadataMsg);
            TAU_STOP("FFS marshaling case");
        }
        Stream->ReaderTimestep = Entry->MetadataMsg->Timestep;
        SstFullMetadata Mdata = malloc(sizeof(struct _SstFullMetadata));
        memset(Mdata, 0, sizeof(struct _SstFullMetadata));
        Mdata->WriterCohortSize = Entry->MetadataMsg->CohortSize;
        Mdata->WriterMetadata =
            malloc(sizeof(Mdata->WriterMetadata[0]) * Mdata->WriterCohortSize);
        for (int i = 0; i < Mdata->WriterCohortSize; i++)
        {
            Mdata->WriterMetadata[i] = &Entry->MetadataMsg->Metadata[i];
        }
        if (Stream->DP_Interface->TimestepInfoFormats == NULL)
        {
            // DP didn't provide struct info, no valid data
            Mdata->DP_TimestepInfo = NULL;
        }
        else
        {
            Mdata->DP_TimestepInfo = Entry->MetadataMsg->DP_TimestepInfo;
        }
        Stream->CurrentWorkingTimestep = Entry->MetadataMsg->Timestep;
        Stream->CurrentMetadata = Mdata;

        CP_verbose(Stream, "SstAdvanceStep returning Success on timestep %d\n",
                   Entry->MetadataMsg->Timestep);
        return SstSuccess;
    }
    if (Stream->Status == PeerClosed)
    {
        CP_verbose(Stream,
                   "SstAdvanceStepPeer returning EndOfStream at timestep %d\n",
                   Stream->ReaderTimestep);
        return SstEndOfStream;
    }
    else
    {
        CP_verbose(Stream,
                   "SstAdvanceStep returning FatalError at timestep %d\n",
                   Stream->ReaderTimestep);
        return SstFatalError;
    }
}

static SstStatusValue SstAdvanceStepMin(SstStream Stream, SstStepMode mode,
                                        const float timeout_sec)
{
    TSMetadataDistributionMsg ReturnData;
    struct _TimestepMetadataMsg *MetadataMsg;
    SstStatusValue ret;

    void *free_block;

    if (Stream->Rank == 0)
    {
        struct _TimestepMetadataDistributionMsg msg;
        SstStatusValue return_value = SstSuccess;
        TSMetadataList RootEntry = NULL;

        memset(&msg, 0, sizeof(msg));
        msg.TSmsg = NULL;
        msg.CommPatternLockedTimestep = -1;
        if (Stream->CommPatternLocked == 1)
        {
            msg.CommPatternLockedTimestep = Stream->CommPatternLockedTimestep;
        }
        if ((timeout_sec >= 0.0) || (mode == SstLatestAvailable))
        {
            long NextTimestep = -1;
            long LatestTimestep = MaxQueuedMetadata(Stream);
            /*
             * Several situations are possible here, depending upon
             * whether or not a timeout is specified and/or
             * LatestAvailable is specified, and whether or not we
             * have timesteps queued anywhere.  If they want
             * LatestAvailable and we have any Timesteps queued
             * anywhere, we decide upon a timestep to return and
             * assume that all ranks will get it soon (or else we're
             * in failure mode).  If there are no timesteps queued
             * anywhere, then we're going to wait for timeout seconds
             * ON RANK 0.  RANK 0 AND ONLY RANK 0 WILL DECIDE IF WE
             * TIMEOUT OR RETURN WITH DATA.  It is possible that other
             * ranks get timestep metadata before the timeout expires,
             * but we don't care.  Whatever would happen on rank 0 is
             * what happens everywhere.
             */

            if (LatestTimestep == -1)
            {
                // AllQueuesEmpty
                if (timeout_sec >= 0.0)
                {
                    waitForMetadataWithTimeout(Stream, timeout_sec);
                }
                else
                {
                    waitForMetadataWithTimeout(Stream, FLT_MAX);
                }
                NextTimestep =
                    MaxQueuedMetadata(Stream); /* might be -1 if we timed out */
            }
            else
            {
                if (mode == SstLatestAvailable)
                {
                    // latest available
                    CP_verbose(Stream,
                               "Returning latest timestep available "
                               "%ld because LatestAvailable "
                               "specified\n",
                               LatestTimestep);
                    NextTimestep = LatestTimestep;
                }
                else
                {
                    // next available (take the oldest that everyone has)
                    NextTimestep = NextQueuedMetadata(Stream);
                    CP_verbose(Stream,
                               "Returning Smallest timestep available "
                               "%ld because NextAvailable specified\n",
                               NextTimestep);
                }
            }
            if (Stream->Status == PeerFailed)
            {
                CP_verbose(Stream,
                           "SstAdvanceStepMin returning FatalError because of "
                           "conn failure at timestep %d\n",
                           Stream->ReaderTimestep);
                return_value = SstFatalError;
            }
            else if ((NextTimestep == -1) && (Stream->Status == PeerClosed))
            {
                CP_verbose(
                    Stream,
                    "SstAdvanceStepMin returning EndOfStream at timestep %d\n",
                    Stream->ReaderTimestep);
                return_value = SstEndOfStream;
            }
            else if (NextTimestep == -1)
            {
                CP_verbose(Stream, "AdvancestepMin timing out on no data\n");
                return_value = SstTimeout;
            }
            else if (mode == SstLatestAvailable)
            {
                // latest available
                /* release all timesteps from before NextTimestep, then fall
                 * through below */
                /* Side note: It is possible that someone could get a "prior"
                 * timestep after this point.  It has to be released upon
                 * arrival */
                CP_verbose(
                    Stream,
                    "timed or Latest timestep, determined NextTimestep %d\n",
                    NextTimestep);
                Stream->DiscardPriorTimestep = NextTimestep;
                releasePriorTimesteps(Stream, NextTimestep);
            }
        }
        if (Stream->Status == PeerFailed)
        {
            CP_verbose(Stream,
                       "SstAdvanceStepMin returning FatalError because of "
                       "conn failure at timestep %d\n",
                       Stream->ReaderTimestep);
            return_value = SstFatalError;
        }
        if (return_value == SstSuccess)
        {
            RootEntry = waitForNextMetadata(Stream, Stream->ReaderTimestep);
        }
        if (RootEntry)
        {
            msg.TSmsg = RootEntry->MetadataMsg;
            msg.ReturnValue = return_value;
            CP_verbose(Stream, "Setting TSmsg to Rootentry value\n");
        }
        else
        {
            if (return_value == SstSuccess)
            {
                if (Stream->Status == PeerClosed)
                {
                    CP_verbose(Stream,
                               "SstAdvanceStepMin rank 0 returning "
                               "EndOfStream at timestep %d\n",
                               Stream->ReaderTimestep);
                    msg.ReturnValue = SstEndOfStream;
                }
                else
                {
                    CP_verbose(Stream,
                               "SstAdvanceStepMin rank 0 returning "
                               "FatalError at timestep %d\n",
                               Stream->ReaderTimestep);
                    msg.ReturnValue = SstFatalError;
                }
                CP_verbose(Stream, "Setting TSmsg to NULL\n");
                msg.TSmsg = NULL;
            }
            else
            {
                msg.ReturnValue = return_value;
            }
        }
        //        AddArrivedMetadataInfo(Stream, &msg);
        ReturnData = CP_distributeDataFromRankZero(
            Stream, &msg, Stream->CPInfo->TimestepDistributionFormat,
            &free_block);
    }
    else
    {
        ReturnData = CP_distributeDataFromRankZero(
            Stream, NULL, Stream->CPInfo->CombinedWriterInfoFormat,
            &free_block);
    }
    ret = ReturnData->ReturnValue;

    //    NotifyDPArrivedMetadataMin(Stream, ReturnData);

    if (ReturnData->ReturnValue != SstSuccess)
    {
        if ((Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS) &&
            (ReturnData->TSmsg))
        {
            CP_verbose(
                Stream,
                "SstAdvanceStep installing precious metadata before exiting\n");
            FFSMarshalInstallPreciousMetadata(Stream, ReturnData->TSmsg);
        }

        free(free_block);
        CP_verbose(Stream, "SstAdvanceStep returning FAILURE\n");
        return ret;
    }
    MetadataMsg = ReturnData->TSmsg;
    if (ReturnData->CommPatternLockedTimestep != -1)
    {
        Stream->CommPatternLockedTimestep =
            ReturnData->CommPatternLockedTimestep;
        Stream->CommPatternLocked = 2;
        if (Stream->DP_Interface->RSreadPatternLocked)
        {
            Stream->DP_Interface->RSreadPatternLocked(
                &Svcs, Stream->DP_Stream, Stream->CommPatternLockedTimestep);
        }
    }
    if (MetadataMsg)
    {
        if (Stream->WriterConfigParams->MarshalMethod == SstMarshalFFS)
        {
            CP_verbose(
                Stream,
                "Calling install precious metadata from metadata block %p\n",
                MetadataMsg);
            FFSMarshalInstallMetadata(Stream, MetadataMsg);
        }
        Stream->ReaderTimestep = MetadataMsg->Timestep;
        SstFullMetadata Mdata = malloc(sizeof(struct _SstFullMetadata));
        memset(Mdata, 0, sizeof(struct _SstFullMetadata));
        Mdata->WriterCohortSize = MetadataMsg->CohortSize;
        Mdata->WriterMetadata =
            malloc(sizeof(Mdata->WriterMetadata[0]) * Mdata->WriterCohortSize);
        for (int i = 0; i < Mdata->WriterCohortSize; i++)
        {
            Mdata->WriterMetadata[i] = &MetadataMsg->Metadata[i];
        }
        if (Stream->DP_Interface->TimestepInfoFormats == NULL)
        {
            // DP didn't provide struct info, no valid data
            Mdata->DP_TimestepInfo = NULL;
        }
        else
        {
            Mdata->DP_TimestepInfo = MetadataMsg->DP_TimestepInfo;
        }
        Stream->CurrentWorkingTimestep = MetadataMsg->Timestep;
        Mdata->FreeBlock = free_block;
        Stream->CurrentMetadata = Mdata;

        CP_verbose(Stream, "SstAdvanceStep returning Success on timestep %d\n",
                   MetadataMsg->Timestep);
        return SstSuccess;
    }
    CP_verbose(Stream, "SstAdvanceStep final return\n");
    return ret;
}

extern SstStatusValue SstAdvanceStep(SstStream Stream, const float timeout_sec)
{

    if (Stream->CurrentMetadata != NULL)
    {
        if (Stream->CurrentMetadata->FreeBlock)
        {
            free(Stream->CurrentMetadata->FreeBlock);
        }
        if (Stream->CurrentMetadata->WriterMetadata)
        {
            free(Stream->CurrentMetadata->WriterMetadata);
        }
        free(Stream->CurrentMetadata);
        Stream->CurrentMetadata = NULL;
    }

    SstStepMode mode = SstNextAvailable;
    if (Stream->ConfigParams->AlwaysProvideLatestTimestep)
    {
        mode = SstLatestAvailable;
    }
    if (Stream->WriterConfigParams->CPCommPattern == SstCPCommPeer)
    {
        return SstAdvanceStepPeer(Stream, mode, timeout_sec);
    }
    else
    {
        return SstAdvanceStepMin(Stream, mode, timeout_sec);
    }
}

extern void SstReaderClose(SstStream Stream)
{
    /* need to have a reader-side shutdown protocol, but for now, just sleep for
     * a little while to makes sure our release message for the last timestep
     * got received */
    struct timeval CloseTime, Diff;
    struct _ReaderCloseMsg Msg;
    /* wait until each reader rank has done SstReaderClose() */
    SMPI_Barrier(Stream->mpiComm);
    gettimeofday(&CloseTime, NULL);
    timersub(&CloseTime, &Stream->ValidStartTime, &Diff);
    memset(&Msg, 0, sizeof(Msg));
    sendOneToEachWriterRank(Stream, Stream->CPInfo->ReaderCloseFormat, &Msg,
                            &Msg.WSR_Stream);
    if (Stream->Stats)
        Stream->Stats->ValidTimeSecs = (double)Diff.tv_usec / 1e6 + Diff.tv_sec;

    CMusleep(Stream->CPInfo->cm, 100000);
    if (Stream->CurrentMetadata != NULL)
    {
        if (Stream->CurrentMetadata->FreeBlock)
            free(Stream->CurrentMetadata->FreeBlock);
        if (Stream->CurrentMetadata->WriterMetadata)
            free(Stream->CurrentMetadata->WriterMetadata);
        free(Stream->CurrentMetadata);
        Stream->CurrentMetadata = NULL;
    }
}

extern SstStatusValue SstWaitForCompletion(SstStream Stream, void *handle)
{
    if (Stream->DP_Interface->waitForCompletion(&Svcs, handle) != 1)
    {
        return SstFatalError;
    }
    else
    {
        return SstSuccess;
    }
}
