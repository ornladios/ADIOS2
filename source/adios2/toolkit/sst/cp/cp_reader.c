#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <atl.h>
#include <evpath.h>
#include <mpi.h>
#include <pthread.h>

#include "sst.h"

#include "cp_internal.h"

static char *readContactInfoFile(const char *Name, SstStream Stream)
{
    size_t len = strlen(Name) + strlen(SST_POSTFIX) + 1;
    char *FileName = malloc(len);
    FILE *WriterInfo;
    snprintf(FileName, len, "%s" SST_POSTFIX, Name);
//    printf("Looking for writer contact in file %s\n", FileName);
redo:
    WriterInfo = fopen(FileName, "r");
    while (!WriterInfo)
    {
        CMusleep(Stream->CPInfo->cm, 500);
        WriterInfo = fopen(FileName, "r");
    }
    struct stat Buf;
    fstat(fileno(WriterInfo), &Buf);
    int Size = Buf.st_size;
    if (Size == 0)
    {
        //  Try again, it might look zero momentarily, but shouldn't stay that
        //  way.
        goto redo;
    }

    free(FileName);
    char *Buffer = calloc(1, Size + 1);
    (void)fread(Buffer, Size, 1, WriterInfo);
    fclose(WriterInfo);
    return Buffer;
}

static char *readContactInfoScreen(const char *Name, SstStream Stream)
{
    char Input[10240];
    char *Skip = Input;
    fprintf(stdout, "Please enter the contact information associated with SST "
                    "input stream \"%s\":\n",
            Name);
    fgets(Input, sizeof(Input), stdin);
    while (isspace(*Skip))
        Skip++;
    return strdup(Skip);
}

static char *readContactInfo(const char *Name, SstStream Stream)
{
    switch (Stream->RegistrationMethod)
    {
    case SstRegisterFile:
        return readContactInfoFile(Name, Stream);
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

static void ReaderConnCloseHandler(CManager cm, CMConnection closed_conn,
                                   void *client_data)
{
    SstStream Stream = (SstStream)client_data;

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
        Stream->Status = PeerClosed;
    }
    else if ((Stream->Status == PeerClosed) || (Stream->Status == Closed))
    {
        /* ignore this.  We expect a close after the connection is marked closed
         */
        CP_verbose(Stream, "Reader-side Rank received a "
                           "connection-close event after close, "
                           "not unexpected\n");
    }
    else
    {
        fprintf(stderr, "Got an unexpected connection close event\n");
        CP_verbose(Stream, "Writer-side Rank received a "
                           "connection-close event in unexpected "
                           "state %d\n",
                   Stream->Status);
        Stream->Status = PeerFailed;
    }
}

extern long SstCurrentStep(SstStream Stream) { return Stream->ReaderTimestep; }

static void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
                                    void **WS_StreamPtr);

static void **ParticipateInReaderInitDataExchange(SstStream Stream,
                                                  void *dpInfo,
                                                  void **ret_data_block)
{

    struct _CP_DP_PairInfo combined_init;
    struct _CP_ReaderInitInfo cpInfo;

    struct _CP_DP_PairInfo **pointers = NULL;

    cpInfo.ContactInfo =
        attr_list_to_string(CMget_contact_list(Stream->CPInfo->cm));
    cpInfo.ReaderID = Stream;

    combined_init.CP_Info = (void **)&cpInfo;
    combined_init.DP_Info = dpInfo;

    pointers = (struct _CP_DP_PairInfo **)CP_consolidateDataToRankZero(
        Stream, &combined_init, Stream->CPInfo->PerRankReaderInfoFormat,
        ret_data_block);
    return (void **)pointers;
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
    int i;
    char *Filename = strdup(Name);
    CMConnection rank0_to_rank0_conn = NULL;

    Stream = CP_newStream();
    Stream->Role = ReaderRole;

    CP_validateParams(Stream, Params, 0 /* reader */);

    Stream->DP_Interface = LoadDP(Stream->DataTransport);

    Stream->CPInfo = CP_getCPInfo(Stream->DP_Interface);

    Stream->mpiComm = comm;
    Stream->FinalTimestep = INT_MAX; /* set this on close */

    MPI_Comm_rank(Stream->mpiComm, &Stream->Rank);
    MPI_Comm_size(Stream->mpiComm, &Stream->CohortSize);

    Stream->DP_Stream =
        Stream->DP_Interface->initReader(&Svcs, Stream, &dpInfo);

    pointers = (struct _CP_DP_PairInfo **)ParticipateInReaderInitDataExchange(
        Stream, dpInfo, &data_block);

    gettimeofday(&Start, NULL);

    if (Stream->Rank == 0)
    {
        char *Writer0Contact = readContactInfo(Filename, Stream);
        void *WriterFileID;
        char *CMContactString =
            malloc(strlen(Writer0Contact)); /* at least long enough */
        struct _CombinedWriterInfo WriterData;

        memset(&WriterData, 0, sizeof(WriterData));
        sscanf(Writer0Contact, "%p:%s", &WriterFileID, CMContactString);
        //        printf("Writer contact info is fileID %p, contact info %s\n",
        //               WriterFileID, CMContactString);
        free(Writer0Contact);

        attr_list WriterRank0Contact = attr_list_from_string(CMContactString);
        CMConnection conn = CMget_conn(Stream->CPInfo->cm, WriterRank0Contact);
        free_attr_list(WriterRank0Contact);

        if (conn)
        {
            /* success!   We have a connection to the writer! */
            struct _ReaderRegisterMsg ReaderRegister;

            ReaderRegister.WriterFile = WriterFileID;
            ReaderRegister.WriterResponseCondition =
                CMCondition_get(Stream->CPInfo->cm, conn);
            ReaderRegister.ReaderCohortSize = Stream->CohortSize;
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

            CMwrite(conn, Stream->CPInfo->ReaderRegisterFormat,
                    &ReaderRegister);
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

            assert(response);
            WriterData.WriterCohortSize = response->WriterCohortSize;
            WriterData.WriterConfigParams = response->WriterConfigParams;
            WriterData.StartingStepNumber = response->NextStepNumber;
            WriterData.CP_WriterInfo = response->CP_WriterInfo;
            WriterData.DP_WriterInfo = response->DP_WriterInfo;
            rank0_to_rank0_conn = conn;
        }
        else
        {
            /* there was no contact with the writer at that location, notify the
             * other ranks */
            WriterData.WriterCohortSize = -1;
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

    if (ReturnData->WriterCohortSize == -1)
    {
        /* Rank 0 found no writer at that contact point, fail the stream */
        free(free_block);
        return NULL;
    }

    //    printf("I am reader rank %d, my info on writers is:\n", Stream->Rank);
    //    FMdump_data(FMFormat_of_original(Stream->CPInfo->combined_writer_Format),
    //                ReturnData, 1024000);
    //    printf("\n");

    Stream->WriterCohortSize = ReturnData->WriterCohortSize;
    Stream->WriterConfigParams = ReturnData->WriterConfigParams;
    if (Stream->WriterConfigParams->FFSmarshal)
    {
        CP_verbose(Stream, "Writer is doing FFS-based marshalling\n");
    }
    if (Stream->WriterConfigParams->BPmarshal)
    {
        CP_verbose(Stream, "Writer is doing BP-based marshalling\n");
    }
    Stream->ReaderTimestep = ReturnData->StartingStepNumber - 1;
    Stream->ConnectionsToWriter =
        calloc(sizeof(CP_PeerConnection), ReturnData->WriterCohortSize);
    for (i = 0; i < ReturnData->WriterCohortSize; i++)
    {
        attr_list attrs =
            attr_list_from_string(ReturnData->CP_WriterInfo[i]->ContactInfo);
        Stream->ConnectionsToWriter[i].ContactList = attrs;
        Stream->ConnectionsToWriter[i].RemoteStreamID =
            ReturnData->CP_WriterInfo[i]->WriterID;
    }

    Stream->Peers = setupPeerArray(Stream->CohortSize, Stream->Rank,
                                   ReturnData->WriterCohortSize);
    i = 0;
    while (Stream->Peers[i] != -1)
    {
        int peer = Stream->Peers[i];
        Stream->ConnectionsToWriter[peer].CMconn = CMget_conn(
            Stream->CPInfo->cm, Stream->ConnectionsToWriter[peer].ContactList);
        CMconn_register_close_handler(Stream->ConnectionsToWriter[peer].CMconn,
                                      ReaderConnCloseHandler, (void *)Stream);
        i++;
    }

    // Deref the original connection to writer rank 0 (might still be open as a
    // peer)
    if (rank0_to_rank0_conn)
    {
        CMConnection_close(rank0_to_rank0_conn);
    }

    Stream->DP_Interface->provideWriterDataToReader(
        &Svcs, Stream->DP_Stream, ReturnData->WriterCohortSize,
        Stream->ConnectionsToWriter, ReturnData->DP_WriterInfo);
    pthread_mutex_lock(&Stream->DataLock);
    Stream->Status = Established;
    pthread_mutex_unlock(&Stream->DataLock);
    CP_verbose(Stream, "Sending Reader Activate messages to writer\n");
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

extern void SstReaderGetParams(SstStream Stream, int *WriterFFSmarshal,
                               int *WriterBPmarshal)
{
    *WriterFFSmarshal = Stream->WriterConfigParams->FFSmarshal;
    *WriterBPmarshal = Stream->WriterConfigParams->BPmarshal;
}

void queueTimestepMetadataMsgAndNotify(SstStream Stream,
                                       struct _TimestepMetadataMsg *tsm,
                                       CMConnection conn)
{
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
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
}

void CP_TimestepMetadataHandler(CManager cm, CMConnection conn, void *Msg_v,
                                void *client_data, attr_list attrs)
{
    SstStream Stream;
    struct _TimestepMetadataMsg *Msg = (struct _TimestepMetadataMsg *)Msg_v;
    Stream = (SstStream)Msg->RS_Stream;
    if (Msg->Metadata == NULL)
    {
        CP_verbose(Stream,
                   "Received a message that timestep %d has been discarded\n",
                   Msg->Timestep);
        /* Nothing to do with this */
        return;
    }
    else
    {
        CP_verbose(Stream,
                   "Received an incoming metadata message for timestep %d\n",
                   Msg->Timestep);
    }
    /* arrange for this message data to stay around */
    CMtake_buffer(cm, Msg);

    queueTimestepMetadataMsgAndNotify(Stream, Msg, conn);
}

void CP_WriterResponseHandler(CManager cm, CMConnection conn, void *Msg_v,
                              void *client_data, attr_list attrs)
{
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
}

extern void CP_WriterCloseHandler(CManager cm, CMConnection conn, void *Msg_v,
                                  void *client_data, attr_list attrs)
{
    WriterCloseMsg Msg = (WriterCloseMsg)Msg_v;
    SstStream Stream = (SstStream)Msg->RS_Stream;

    CP_verbose(Stream, "Received a writer close message. "
                       "Timestep %d was the final timestep.\n",
               Msg->FinalTimestep);

    pthread_mutex_lock(&Stream->DataLock);
    Stream->FinalTimestep = Msg->FinalTimestep;
    Stream->Status = PeerClosed;
    /* wake anyone that might be waiting */
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
}

static TSMetadataList waitForMetadata(SstStream Stream, long Timestep)
{
    struct _TimestepMetadataList *Next;
    pthread_mutex_lock(&Stream->DataLock);
    Next = Stream->Timesteps;
    while (1)
    {
        Next = Stream->Timesteps;
        while (Next)
        {
            if (Next->MetadataMsg->Timestep == Timestep)
            {
                pthread_mutex_unlock(&Stream->DataLock);
                CP_verbose(Stream, "Returning metadata for Timestep %d\n",
                           Timestep);
                return Next;
            }
            Next = Next->Next;
        }
        /* didn't find requested timestep, check Stream status */
        if (Stream->Status != Established)
        {
            /* closed or failed, return NULL */
            return NULL;
        }
        CP_verbose(Stream, "Waiting for metadata for Timestep %d\n", Timestep);
        /* wait until we get the timestep metadata or something else changes */
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    /* NOTREACHED */
    pthread_mutex_unlock(&Stream->DataLock);
}

static TSMetadataList waitForNextMetadata(SstStream Stream, long LastTimestep)
{
    struct _TimestepMetadataList *Next;
    TSMetadataList FoundTS = NULL;
    pthread_mutex_lock(&Stream->DataLock);
    Next = Stream->Timesteps;
    CP_verbose(Stream, "Wait for next metadata after last timestep %d\n",
               LastTimestep);
    while (1)
    {
        Next = Stream->Timesteps;
        while (Next)
        {
            if (Next->MetadataMsg->Timestep >= LastTimestep)
            {
                if ((FoundTS == NULL) &&
                    (Next->MetadataMsg->Timestep > LastTimestep))
                {
                    FoundTS = Next;
                }
                else if ((FoundTS != NULL) && (FoundTS->MetadataMsg->Timestep >
                                               Next->MetadataMsg->Timestep))
                {
                    FoundTS = Next;
                }
            }
            Next = Next->Next;
        }
        if (FoundTS)
        {
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream, "Returning metadata for Timestep %d\n",
                       FoundTS->MetadataMsg->Timestep);
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
            CP_verbose(Stream, "Wait for next metadata returning NULL because "
                               "Stream is not Established\n");
            /* closed or failed, return NULL */
            return NULL;
        }
        CP_verbose(Stream,
                   "Waiting for metadata for a Timestep later than TS %d\n",
                   LastTimestep);
        CP_verbose(Stream, "Stream status is %d\n", Stream->Status);
        /* wait until we get the timestep metadata or something else changes */
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    /* NOTREACHED */
    pthread_mutex_unlock(&Stream->DataLock);
}

extern SstFullMetadata SstGetMetadata(SstStream Stream, long timestep)
{
    TSMetadataList Entry;
    SstFullMetadata Ret;
    Entry = waitForMetadata(Stream, timestep);
    if (Entry)
    {
        Ret = malloc(sizeof(struct _SstFullMetadata));
        Ret->WriterCohortSize = Entry->MetadataMsg->CohortSize;
        Ret->WriterMetadata = Entry->MetadataMsg->Metadata;
        if (Stream->DP_Interface->TimestepInfoFormats == NULL)
        {
            // DP didn't provide struct info, no valid data
            Ret->DP_TimestepInfo = NULL;
        }
        else
        {
            Ret->DP_TimestepInfo = Entry->MetadataMsg->DP_TimestepInfo;
        }
        Stream->CurrentWorkingTimestep = timestep;
        return Ret;
    }
    assert(Stream->Status != Established);
    return NULL;
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
    int i = 0;
    while (s->Peers[i] != -1)
    {
        int peer = s->Peers[i];
        CMConnection conn = s->ConnectionsToWriter[peer].CMconn;
        /* add the writer Stream identifier to each outgoing
         * message */
        *WS_StreamPtr = s->ConnectionsToWriter[peer].RemoteStreamID;
        CMwrite(conn, f, Msg);
        i++;
    }
}

extern void SstReleaseStep(SstStream Stream)
{
    long MaxTimestep;
    long Timestep = Stream->ReaderTimestep;
    struct _ReleaseTimestepMsg Msg;

    /*
     * remove local metadata for that timestep
     */
    pthread_mutex_lock(&Stream->DataLock);
    struct _TimestepMetadataList *List = Stream->Timesteps;

    if (Stream->Timesteps->MetadataMsg->Timestep == Timestep)
    {
        Stream->Timesteps = List->Next;
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
                free(List);
            }
            last = List;
            List = List->Next;
        }
    }
    pthread_mutex_unlock(&Stream->DataLock);

    /*
     * this can be just a barrier (to ensure that everyone has called
     * SstReleaseStep), but doing a reduce and comparing the returned max to
     * our value will detect if someone is calling with a different timestep
     * value (which would be bad).  This is a relatively cheap upcost from
     * simple barrier in return for robustness checking.
     */
    MPI_Allreduce(&Timestep, &MaxTimestep, 1, MPI_LONG, MPI_MAX,
                  Stream->mpiComm);
    assert((Timestep == MaxTimestep) && "All ranks must be in sync.  Someone "
                                        "called SstReleaseTimestep with a "
                                        "different timestep value");

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

    FFSClearTimestepData(Stream);
}

/*
 * wait for metadata for Timestep indicated to arrive, or fail with EndOfStream
 * or Error
 */
extern SstStatusValue SstAdvanceStep(SstStream Stream, int mode,
                                     const float timeout_sec)
{

    TSMetadataList Entry = waitForNextMetadata(Stream, Stream->ReaderTimestep);

    if (Entry)
    {
        if (Stream->WriterConfigParams->FFSmarshal)
        {
            FFSMarshalInstallMetadata(Stream, Entry->MetadataMsg);
        }
        CP_verbose(Stream, "SstAdvanceStep returning Success on timestep %d\n",
                   Entry->MetadataMsg->Timestep);
        Stream->ReaderTimestep = Entry->MetadataMsg->Timestep;
        return SstSuccess;
    }
    if (Stream->Status == PeerClosed)
    {
        CP_verbose(Stream,
                   "SstAdvanceStep returning EndOfStream at timestep %d\n",
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

extern void SstReaderClose(SstStream Stream)
{
    /* need to have a reader-side shutdown protocol, but for now, just sleep for
     * a little while to makes sure our release message for the last timestep
     * got received */
    struct timeval CloseTime, Diff;
    struct _ReaderCloseMsg Msg;
    /* wait until each reader rank has done SstReaderClose() */
    MPI_Barrier(Stream->mpiComm);
    gettimeofday(&CloseTime, NULL);
    timersub(&CloseTime, &Stream->ValidStartTime, &Diff);
    sendOneToEachWriterRank(Stream, Stream->CPInfo->ReaderCloseFormat, &Msg,
                            &Msg.WSR_Stream);
    if (Stream->Stats)
        Stream->Stats->ValidTimeSecs = (double)Diff.tv_usec / 1e6 + Diff.tv_sec;

    CMsleep(Stream->CPInfo->cm, 1);
}

extern SstStatusValue SstWaitForCompletion(SstStream Stream, void *handle)
{
    //   We need a way to return an error from DP */
    Stream->DP_Interface->waitForCompletion(&Svcs, handle);
    return SstSuccess;
}
