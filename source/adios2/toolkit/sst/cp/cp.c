#include <assert.h>
#include <limits.h>
#include <stdarg.h>
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

extern void CP_verbose(SstStream Stream, char *Format, ...);
static void DP_verbose(SstStream Stream, char *Format, ...);
static CManager CP_getCManager(SstStream Stream);
static void CP_sendToPeer(SstStream Stream, CP_PeerCohort cohort, int rank,
                          CMFormat Format, void *data);
static MPI_Comm CP_getMPIComm(SstStream Stream);

struct _CP_Services Svcs = {
    (CP_VerboseFunc)DP_verbose, (CP_GetCManagerFunc)CP_getCManager,
    (CP_SendToPeerFunc)CP_sendToPeer, (CP_GetMPICommFunc)CP_getMPIComm};

static void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
                                    void **WS_StreamPtr);
static void writeContactInfo(const char *Name, SstStream Stream)
{
    char *Contact = attr_list_to_string(CMget_contact_list(Stream->CPInfo->cm));
    char *TmpName = malloc(strlen(Name) + strlen(".tmp") + 1);
    char *FileName = malloc(strlen(Name) + strlen(".bpflx") + 1);
    FILE *WriterInfo;

    /*
     * write the contact information file with a temporary name before
     * renaming it to the final version to help prevent partial reads
     */
    sprintf(TmpName, "%s.tmp", Name);
    sprintf(FileName, "%s.bpflx", Name);
    WriterInfo = fopen(TmpName, "w");
    fprintf(WriterInfo, "%p:%s", (void *)Stream, Contact);
    fclose(WriterInfo);
    rename(TmpName, FileName);
    free(TmpName);
    free(FileName);
}

static void removeContactInfo(SstStream Stream)
{
    const char *Name = Stream->Filename;
    char *FileName = malloc(strlen(Name) + strlen(".bpflx") + 1);
    FILE *WriterInfo;
    sprintf(FileName, "%s.bpflx", Name);
    unlink(FileName);
}

static char *readContactInfo(const char *Name, SstStream Stream)
{
    char *FileName = malloc(strlen(Name) + strlen(".bpflx") + 1);
    FILE *WriterInfo;
    sprintf(FileName, "%s.bpflx", Name);
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

    char *Buffer = calloc(1, Size + 1);
    (void)fread(Buffer, Size, 1, WriterInfo);
    fclose(WriterInfo);
    return Buffer;
}

static int *setupPeerArray(int MySize, int MyRank, int PeerSize)
{
    int PortionSize = PeerSize / MySize;
    int Leftovers = PeerSize - PortionSize * MySize;
    int StartOffset = Leftovers;
    int Start;
    if (MyRank < Leftovers)
    {
        PortionSize++;
        StartOffset = 0;
    }
    Start = PortionSize * MyRank + StartOffset;
    int *MyPeers = malloc((PortionSize + 1) * sizeof(int));
    for (int i = 0; i < PortionSize; i++)
    {
        MyPeers[i] = Start + i;
    }
    MyPeers[PortionSize] = -1;

    return MyPeers;
}

static void WriterConnCloseHandler(CManager cm, CMConnection closed_conn,
                                   void *client_data)
{
    WS_ReaderInfo WSreader = (WS_ReaderInfo)client_data;
    SstStream ParentWriterStream = WSreader->ParentStream;

    if (WSreader->ReaderStatus == Established)
    {
        /*
         * tag our reader instance as failed.
         * If any instance is failed, we should remove all, but that requires a
         * global operation, so prep.
         */
        CP_verbose(ParentWriterStream, "Writer-side Rank received a "
                                       "connection-close event during normal "
                                       "operations, peer likely failed\n");
        WSreader->ReaderStatus = PeerFailed;
        ParentWriterStream->GlobalOpRequired = 1;
    }
    else if ((WSreader->ReaderStatus == PeerClosed) ||
             (WSreader->ReaderStatus == Closed))
    {
        /* ignore this.  We expect a close after the connection is marked closed
         */
        CP_verbose(ParentWriterStream, "Writer-side Rank received a "
                                       "connection-close event after close, "
                                       "not unexpected\n");
    }
    else
    {
        fprintf(stderr, "Got an unexpected connection close event\n");
        CP_verbose(ParentWriterStream, "Writer-side Rank received a "
                                       "connection-close event in unexpected "
                                       "state %d\n",
                   WSreader->ReaderStatus);
        WSreader->ReaderStatus = PeerFailed;
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

static void initWSReader(WS_ReaderInfo reader, int ReaderSize,
                         CP_ReaderInitInfo *reader_info)
{
    int WriterSize = reader->ParentStream->CohortSize;
    int WriterRank = reader->ParentStream->Rank;
    int i;
    reader->ReaderCohortSize = ReaderSize;
    reader->Connections = calloc(sizeof(reader->Connections[0]), ReaderSize);
    for (i = 0; i < ReaderSize; i++)
    {
        reader->Connections[i].ContactList =
            attr_list_from_string(reader_info[i]->ContactInfo);
        reader->Connections[i].RemoteStreamID = reader_info[i]->ReaderID;
        reader->Connections[i].CMconn = NULL;
    }
    reader->Peers = setupPeerArray(WriterSize, WriterRank, ReaderSize);
    i = 0;
    while (reader->Peers[i] != -1)
    {
        int peer = reader->Peers[i];
        reader->Connections[peer].CMconn =
            CMget_conn(reader->ParentStream->CPInfo->cm,
                       reader->Connections[peer].ContactList);
        CMconn_register_close_handler(reader->Connections[peer].CMconn,
                                      WriterConnCloseHandler, (void *)reader);
        i++;
    }
}

static long earliestAvailableTimestepNumber(SstStream Stream,
                                            long CurrentTimestep)
{
    long Ret = CurrentTimestep;
    CPTimestepList List = Stream->QueuedTimesteps;
    pthread_mutex_lock(&Stream->DataLock);
    List = Stream->QueuedTimesteps;
    while (List)
    {
        if (List->Timestep < Ret)
        {
            Ret = List->Timestep;
        }
        List = List->Next;
    }
    pthread_mutex_unlock(&Stream->DataLock);
    return Ret;
}

static void AddRefRangeTimestep(SstStream Stream, long LowRange, long HighRange)
{
    CPTimestepList List = Stream->QueuedTimesteps;
    pthread_mutex_lock(&Stream->DataLock);
    List = Stream->QueuedTimesteps;
    while (List)
    {
        if ((List->Timestep >= LowRange) && (List->Timestep <= HighRange))
        {
            List->ReferenceCount++;
        }
        List = List->Next;
    }
    pthread_mutex_unlock(&Stream->DataLock);
}

static void SubRefRangeTimestep(SstStream Stream, long LowRange, long HighRange)
{
    CPTimestepList Last = NULL, List;
    int AnythingRemoved = 0;
    pthread_mutex_lock(&Stream->DataLock);
    List = Stream->QueuedTimesteps;
    while (List)
    {
        if ((List->Timestep >= LowRange) && (List->Timestep <= HighRange))
        {
            List->ReferenceCount--;
        }
        if (List->ReferenceCount == 0)
        {
            /* dequeue and free */
            CPTimestepList ItemToFree = List;
            if (Last == NULL)
            {
                Stream->QueuedTimesteps = List->Next;
            }
            else
            {
                Last->Next = List->Next;
            }
            CP_verbose(Stream, "Step %d reference count reached zero, "
                               "releasing from DP and freeing\n",
                       List->Timestep);
            Stream->DP_Interface->releaseTimestep(&Svcs, Stream->DP_Stream,
                                                  List->Timestep);

            Stream->QueuedTimestepCount--;
            List = List->Next;
            free(ItemToFree);
            AnythingRemoved++;
        }
        else
        {
            Last = List;
            List = List->Next;
        }
    }
    if (AnythingRemoved)
    {
        /* main thread might be waiting on timesteps going away */
        pthread_cond_signal(&Stream->DataCondition);
    }
    pthread_mutex_unlock(&Stream->DataLock);
}

static void KillZeroRefTimesteps(SstStream Stream)
{
    CPTimestepList Last = NULL, List;
    int AnythingRemoved = 0;
    pthread_mutex_lock(&Stream->DataLock);
    List = Stream->QueuedTimesteps;
    while (List)
    {
        if (List->ReferenceCount == 0)
        {
            /* dequeue and free */
            CPTimestepList ItemToFree = List;
            if (Last == NULL)
            {
                Stream->QueuedTimesteps = List->Next;
            }
            else
            {
                Last->Next = List->Next;
            }
            CP_verbose(Stream, "Step %d reference count is zero, "
                               "releasing from DP and freeing\n",
                       List->Timestep);
            Stream->DP_Interface->releaseTimestep(&Svcs, Stream->DP_Stream,
                                                  List->Timestep);

            Stream->QueuedTimestepCount--;
            List = List->Next;
            free(ItemToFree);
            AnythingRemoved++;
        }
        else
        {
            Last = List;
            List = List->Next;
        }
    }
    if (AnythingRemoved)
    {
        /* main thread might be waiting on timesteps going away */
        pthread_cond_signal(&Stream->DataCondition);
    }
    pthread_mutex_unlock(&Stream->DataLock);
}

WS_ReaderInfo WriterParticipateInReaderOpen(SstStream Stream)
{
    RequestQueue Req;
    reader_data_t ReturnData;
    void *free_block = NULL;
    int WriterResponseCondition = -1;
    CMConnection conn;
    long MyStartingTimestep, GlobalStartingTimestep;

    CP_verbose(Stream, "Beginning writer-side reader open protocol\n");
    if (Stream->Rank == 0)
    {
        pthread_mutex_lock(&Stream->DataLock);
        assert((Stream->ReadRequestQueue));
        Req = Stream->ReadRequestQueue;
        Stream->ReadRequestQueue = Req->Next;
        Req->Next = NULL;
        pthread_mutex_unlock(&Stream->DataLock);
        struct _CombinedReaderInfo reader_data;
        reader_data.ReaderCohortSize = Req->Msg->ReaderCohortSize;
        reader_data.CP_ReaderInfo = Req->Msg->CP_ReaderInfo;
        reader_data.DP_ReaderInfo = Req->Msg->DP_ReaderInfo;
        ReturnData = CP_distributeDataFromRankZero(
            Stream, &reader_data, Stream->CPInfo->CombinedReaderInfoFormat,
            &free_block);
        WriterResponseCondition = Req->Msg->WriterResponseCondition;
        conn = Req->Conn;
        CMreturn_buffer(Stream->CPInfo->cm, Req->Msg);
        free(Req);
    }
    else
    {
        ReturnData = CP_distributeDataFromRankZero(
            Stream, NULL, Stream->CPInfo->CombinedReaderInfoFormat,
            &free_block);
    }
    //    printf("I am writer rank %d, my info on readers is:\n", Stream->Rank);
    //    FMdump_data(FMFormat_of_original(Stream->CPInfo->combined_reader_Format),
    //                ReturnData, 1024000);
    //    printf("\n");

    Stream->Readers = realloc(Stream->Readers, sizeof(Stream->Readers[0]) *
                                                   (Stream->ReaderCount + 1));
    DP_WSR_Stream per_reader_Stream;
    void *DP_WriterInfo;
    void *ret_data_block;
    CP_PeerConnection *connections_to_reader;
    connections_to_reader =
        calloc(sizeof(CP_PeerConnection), ReturnData->ReaderCohortSize);
    for (int i = 0; i < ReturnData->ReaderCohortSize; i++)
    {
        attr_list attrs =
            attr_list_from_string(ReturnData->CP_ReaderInfo[i]->ContactInfo);
        connections_to_reader[i].ContactList = attrs;
        connections_to_reader[i].RemoteStreamID =
            ReturnData->CP_ReaderInfo[i]->ReaderID;
    }

    per_reader_Stream = Stream->DP_Interface->initWriterPerReader(
        &Svcs, Stream->DP_Stream, ReturnData->ReaderCohortSize,
        connections_to_reader, ReturnData->DP_ReaderInfo, &DP_WriterInfo);

    WS_ReaderInfo CP_WSR_Stream = malloc(sizeof(*CP_WSR_Stream));
    Stream->Readers[Stream->ReaderCount] = CP_WSR_Stream;
    CP_WSR_Stream->DP_WSR_Stream = per_reader_Stream;
    CP_WSR_Stream->ParentStream = Stream;
    CP_WSR_Stream->Connections = connections_to_reader;
    initWSReader(CP_WSR_Stream, ReturnData->ReaderCohortSize,
                 ReturnData->CP_ReaderInfo);

    Stream->ReaderCount++;

    struct _CP_DP_PairInfo combined_init;
    struct _CP_WriterInitInfo cpInfo;

    struct _CP_DP_PairInfo **pointers = NULL;

    cpInfo.ContactInfo =
        attr_list_to_string(CMget_contact_list(Stream->CPInfo->cm));
    cpInfo.WriterID = CP_WSR_Stream;

    combined_init.CP_Info = (void **)&cpInfo;
    combined_init.DP_Info = DP_WriterInfo;

    MyStartingTimestep =
        earliestAvailableTimestepNumber(Stream, Stream->WriterTimestep);
    if (MyStartingTimestep == -1)
        MyStartingTimestep = 0;

    MPI_Allreduce(&MyStartingTimestep, &GlobalStartingTimestep, 1, MPI_LONG,
                  MPI_MAX, Stream->mpiComm);

    AddRefRangeTimestep(Stream, GlobalStartingTimestep, LONG_MAX);

    CP_WSR_Stream->StartingTimestep = GlobalStartingTimestep;

    pointers = (struct _CP_DP_PairInfo **)CP_consolidateDataToRankZero(
        Stream, &combined_init, Stream->CPInfo->PerRankWriterInfoFormat,
        &ret_data_block);

    if (Stream->Rank == 0)
    {
        struct _WriterResponseMsg response;
        memset(&response, 0, sizeof(response));
        response.WriterResponseCondition = WriterResponseCondition;
        response.WriterCohortSize = Stream->CohortSize;
        response.NextStepNumber = GlobalStartingTimestep;
        response.CP_WriterInfo =
            malloc(response.WriterCohortSize * sizeof(void *));
        response.DP_WriterInfo =
            malloc(response.WriterCohortSize * sizeof(void *));
        for (int i = 0; i < response.WriterCohortSize; i++)
        {
            response.CP_WriterInfo[i] =
                (struct _CP_WriterInitInfo *)pointers[i]->CP_Info;
            response.DP_WriterInfo[i] = pointers[i]->DP_Info;
        }
        CMwrite(conn, Stream->CPInfo->WriterResponseFormat, &response);
    }
    Stream->NewReaderPresent = 1;
    CP_verbose(Stream, "Finish writer-side reader open protocol for reader %p, "
                       "reader ready response pending\n",
               CP_WSR_Stream);
    return CP_WSR_Stream;
}

void sendOneToWSRCohort(WS_ReaderInfo CP_WSR_Stream, CMFormat f, void *Msg,
                        void **RS_StreamPtr)
{
    SstStream s = CP_WSR_Stream->ParentStream;
    int j = 0;

    while (CP_WSR_Stream->Peers[j] != -1)
    {
        int peer = CP_WSR_Stream->Peers[j];
        CMConnection conn = CP_WSR_Stream->Connections[peer].CMconn;
        /* add the reader-rank-specific Stream identifier to each outgoing
         * message */
        *RS_StreamPtr = CP_WSR_Stream->Connections[peer].RemoteStreamID;
        CP_verbose(s, "Sending a message to reader %d\n", peer);
        CMwrite(conn, f, Msg);
        j++;
    }
}

static void waitForReaderResponseAndSendQueued(WS_ReaderInfo Reader)
{
    SstStream Stream = Reader->ParentStream;
    pthread_mutex_lock(&Stream->DataLock);
    while (Reader->ReaderStatus != Established)
    {
        CP_verbose(Stream, "Waiting for Reader ready on WSR %p.\n", Reader);
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    assert(Reader->ReaderStatus == Established);
    pthread_mutex_unlock(&Stream->DataLock);

    /* send any queued metadata necessary */
    CP_verbose(Stream, "Reader ready on WSR %p, Stream established.\n", Reader);
    for (long TS = Reader->StartingTimestep; TS <= Stream->LastProvidedTimestep;
         TS++)
    {
        CPTimestepList List = Stream->QueuedTimesteps;
        while (List)
        {
            if (List->Timestep == TS)
            {
                FFSFormatList SavedFormats = List->Msg->Formats;
                CP_verbose(Stream,
                           "Sending Queued TimestepMetadata for timestep %d\n",
                           TS);

                if (TS == Reader->StartingTimestep)
                {
                    /* For first Msg, send all previous formats */
                    List->Msg->Formats = Stream->PreviousFormats;
                }
                sendOneToWSRCohort(
                    Reader, Stream->CPInfo->DeliverTimestepMetadataFormat,
                    List->Msg, &List->Msg->RS_Stream);
                if (TS == Reader->StartingTimestep)
                {
                    /* restore Msg format list */
                    List->Msg->Formats = SavedFormats;
                }
            }
            List = List->Next;
        }
    }
}

static char *TrimSuffix(const char *Name)
{
    char *Ret = strdup(Name);
    int Len = strlen(Name);
    if (strcmp(Name + Len - 3, ".bp") == 0)
        Ret[Len - 3] = 0;
    return Ret;
}

SstStream SstWriterOpen(const char *Name, SstParams Params, MPI_Comm comm)
{
    SstStream Stream;

    Stream = CP_newStream();
    Stream->Role = WriterRole;
    CP_validateParams(Stream, Params, 1 /* Writer */);

    char *Filename = TrimSuffix(Name);
    Stream->DP_Interface = LoadDP("dummy");

    Stream->CPInfo = CP_getCPInfo(Stream->DP_Interface);

    Stream->mpiComm = comm;
    if (Stream->RendezvousReaderCount > 0)
    {
        Stream->FirstReaderCondition =
            CMCondition_get(Stream->CPInfo->cm, NULL);
    }
    else
    {
        Stream->FirstReaderCondition = -1;
    }

    MPI_Comm_rank(Stream->mpiComm, &Stream->Rank);
    MPI_Comm_size(Stream->mpiComm, &Stream->CohortSize);

    Stream->DP_Stream = Stream->DP_Interface->initWriter(&Svcs, Stream);

    if (Stream->Rank == 0)
    {
        writeContactInfo(Filename, Stream);
    }

    CP_verbose(Stream, "Opening Stream \"%s\"\n", Filename);

    while (Stream->RendezvousReaderCount > 0)
    {
        WS_ReaderInfo reader;
        CP_verbose(Stream, "Stream \"%s\" waiting for %d readers\n", Filename,
                   Stream->RendezvousReaderCount);
        if (Stream->Rank == 0)
        {
            pthread_mutex_lock(&Stream->DataLock);
            if (Stream->ReadRequestQueue == NULL)
            {
                pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
            }
            assert(Stream->ReadRequestQueue);
            pthread_mutex_unlock(&Stream->DataLock);
        }
        MPI_Barrier(Stream->mpiComm);

        struct timeval Start, Stop, Diff;
        gettimeofday(&Start, NULL);
        reader = WriterParticipateInReaderOpen(Stream);
        waitForReaderResponseAndSendQueued(reader);
        gettimeofday(&Stop, NULL);
        timersub(&Stop, &Start, &Diff);
        Stream->OpenTimeSecs = (double)Diff.tv_usec / 1e6 + Diff.tv_sec;
        MPI_Barrier(Stream->mpiComm);
        gettimeofday(&Stream->ValidStartTime, NULL);
        Stream->RendezvousReaderCount--;
    }
    Stream->Filename = Filename;
    CP_verbose(Stream, "Finish opening Stream \"%s\"\n", Filename);
    return Stream;
}

void sendOneToEachReaderRank(SstStream s, CMFormat f, void *Msg,
                             void **RS_StreamPtr)
{
    for (int i = 0; i < s->ReaderCount; i++)
    {
        WS_ReaderInfo CP_WSR_Stream = s->Readers[i];
        if (CP_WSR_Stream->ReaderStatus == Established)
        {
            CP_verbose(s, "Working on reader cohort %d\n", i);
        }
        else
        {
            CP_verbose(s, "Skipping reader cohort %d\n", i);
            continue;
        }
        sendOneToWSRCohort(CP_WSR_Stream, f, Msg, RS_StreamPtr);
    }
}

void SstWriterClose(SstStream Stream)
{
    struct _WriterCloseMsg Msg;
    struct timeval CloseTime, Diff;
    Msg.FinalTimestep = Stream->LastProvidedTimestep;
    sendOneToEachReaderRank(Stream, Stream->CPInfo->WriterCloseFormat, &Msg,
                            &Msg.RS_Stream);

    KillZeroRefTimesteps(Stream);

    /* wait until all queued data is sent */
    pthread_mutex_lock(&Stream->DataLock);
    while (Stream->QueuedTimesteps)
    {
        CP_verbose(Stream,
                   "Waiting for timesteps to be released in WriterClose\n");
        CP_verbose(Stream, "The first timestep still queued is %d\n",
                   Stream->QueuedTimesteps->Timestep);
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    pthread_mutex_unlock(&Stream->DataLock);

    gettimeofday(&CloseTime, NULL);
    timersub(&CloseTime, &Stream->ValidStartTime, &Diff);
    if (Stream->Stats)
        Stream->Stats->ValidTimeSecs = (double)Diff.tv_usec / 1e6 + Diff.tv_sec;

    CP_verbose(Stream, "All timesteps are released in WriterClose\n");
    /*
     *  We'll go ahead and remove the contact info since multiple
     *  readers is not yet implemented
     */
    removeContactInfo(Stream);
}

static FFSFormatList AddUniqueFormats(FFSFormatList List,
                                      FFSFormatList Candidates)
{
    FFSFormatList Tmp = List;
    FFSFormatList Ret = List;

    // If nothing to add, return original
    if (!Candidates)
        return Ret;

    // Add tail of candidates list first
    Ret = AddUniqueFormats(List, Candidates->Next);

    while (Tmp)
    {
        if ((Tmp->FormatIDRepLen == Candidates->FormatIDRepLen) &&
            (memcmp(Tmp->FormatIDRep, Candidates->FormatIDRep,
                    Tmp->FormatIDRepLen) == 0))
        {
            // Identical format already in List, don't add this one
            return Ret;
        }
        Tmp = Tmp->Next;
    }
    // New format not in list, add him to head and return.
    // This is destructive of candidates list, but that is unimportant for
    // deallocation in this circumstance.
    Candidates->Next = Ret;
    return Candidates;
}

static void DoWriterSideGlobalOp(SstStream Stream)
{
    int SendBlockSize = Stream->ReaderCount + 1;
    int *SendBlock = malloc(sizeof(SendBlock[0]) * SendBlockSize);
    int *RecvBlock =
        malloc(sizeof(SendBlock[0]) * SendBlockSize * Stream->CohortSize);
    CP_verbose(Stream, "Initiating Writer Side global operation\n");
    Stream->GlobalOpRequired = 0; //  reset if set
    RequestQueue ArrivingReader = Stream->ReadRequestQueue;

    /* build local info item -- contents:
     *     Reader_requests_join (integer, contains count, only rank 0 should
     *ever set)
     *	   Reader_status (integer, repeated ReaderCount times)
     */
    SendBlock[0] = 0;
    while (ArrivingReader)
    {
        SendBlock[0]++;
        ArrivingReader = ArrivingReader->Next;
    }
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        SendBlock[i + 1] = Stream->Readers[i]->ReaderStatus;
    }
    MPI_Allgather(SendBlock, SendBlockSize, MPI_INT, RecvBlock, SendBlockSize,
                  MPI_INT, Stream->mpiComm);

    /*
     *  Handle incoming data in a deterministic way.  First handle any changes
     * in state for existing clients.
     */

    for (int i = 0; i < Stream->ReaderCount; i++)
    {

        if (Stream->Readers[i]->ReaderStatus == Established)
        {
            /*
             * if we think a reader is OK, see if anyone has had a failure with
             * it
             */
            for (int j = 0; j < Stream->CohortSize; j++)
            {
                if (RecvBlock[i * SendBlockSize + j + 1] == PeerFailed)
                {
                    Stream->Readers[i]->ReaderStatus = PeerFailed;
                }
            }
        }
        if (Stream->Readers[i]->ReaderStatus == PeerFailed)
        {
            // If we see PeerFailed now, everyone does, move to Fail
            CP_verbose(Stream, "Reader %d now determined to have failed, "
                               "dereferencing the timesteps it was sent, %d to "
                               "%d\n",
                       i, Stream->Readers[i]->StartingTimestep,
                       Stream->Readers[i]->LastSentTimestep);
            Stream->Readers[i]->ReaderStatus = Closed;
            SubRefRangeTimestep(Stream, Stream->Readers[i]->StartingTimestep,
                                Stream->Readers[i]->LastSentTimestep);
        }
    }

    /*
     *  Then handle possible incoming connection requests.  (Only rank 0 has
     * valid info.)
     */
    for (int i = 0; i < RecvBlock[0]; i++)
    {
        WS_ReaderInfo reader;
        CP_verbose(Stream,
                   "Writer side Global operation accepting incoming reader\n");
        reader = WriterParticipateInReaderOpen(Stream);
        waitForReaderResponseAndSendQueued(reader);
    }

    free(SendBlock);
    free(RecvBlock);
    CP_verbose(Stream, "Finished with Writer side Global operations\n");
}

static long DoStreamDiscard(SstStream Stream, long IncomingTimestep)
{
    /* Stream data is locked on entry */
    int ReaderCount = 0;
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        if (Stream->Readers[i]->ReaderStatus == Established)
        {
            ReaderCount++;
        }
    }
    if (ReaderCount == 0)
    {
        /* No readers, discard earliest timestep in queue (at the end of TS
         * list) */
        if (!Stream->QueuedTimesteps)
        {
            fprintf(stderr, "Expected at least one timestep to dequeue!\n");
        }
        if (Stream->QueuedTimesteps->Next == NULL)
        {
            CPTimestepList Entry = Stream->QueuedTimesteps;
            if (Stream->QueuedTimesteps->ReferenceCount != 0)
            {
                fprintf(stderr,
                        "\n\n\nREFERENCE COUNT CONDITION VIOLATION\n\n\n\n");
            }
            CP_verbose(Stream,
                       "Discarding timestep %d because of queue condition\n",
                       Entry->Timestep);
            Stream->QueuedTimesteps = NULL;
            Entry->DataFreeFunc(Entry->FreeClientData);
            free(Entry);
            Stream->QueuedTimestepCount--;
        }
        else
        {
            CPTimestepList Last = Stream->QueuedTimesteps;
            while (Last->Next->Next != NULL)
            {
                Last = Last->Next;
            }
            CPTimestepList Entry = Last->Next;
            if (Entry->ReferenceCount != 0)
            {
                fprintf(stderr,
                        "\n\n\nREFERENCE COUNT CONDITION VIOLATION\n\n\n\n");
            }
            CP_verbose(Stream,
                       "Discarding timestep %d because of queue condition\n",
                       Entry->Timestep);
            Last->Next = NULL;
            Entry->DataFreeFunc(Entry->FreeClientData);
            free(Entry);
            Stream->QueuedTimestepCount--;
        }
        return 0;
    }
    else
    {
        /* we have readers, all metadata has been sent to them with no easy way
         * to retrieve it, just discard incoming */
        CP_verbose(
            Stream,
            "Discarding incoming timestep %d because of queue full condition\n",
            IncomingTimestep);
        return 1;
    }
}

extern void SstInternalProvideTimestep(SstStream Stream, SstData LocalMetadata,
                                       SstData Data, long Timestep,
                                       FFSFormatList Formats,
                                       void DataFreeFunc(void *),
                                       void *FreeClientData)
{
    void *data_block;
    MetadataPlusDPInfo *pointers;
    struct _TimestepMetadataMsg *Msg = malloc(sizeof(*Msg));
    void *DP_TimestepInfo = NULL;
    struct _MetadataPlusDPInfo Md;
    CPTimestepList Entry = calloc(1, sizeof(struct _CPTimestepEntry));
    FFSFormatList XmitFormats = NULL;
    int GlobalOpRequested = 0;

    pthread_mutex_lock(&Stream->DataLock);
    if ((Stream->QueueLimit > 0) &&
        (Stream->QueuedTimestepCount >= Stream->QueueLimit))
    {
        if (Stream->DiscardOnQueueFull)
        {
            int IncomingTimestepDiscarded;
            IncomingTimestepDiscarded = DoStreamDiscard(Stream, Timestep);
            if (IncomingTimestepDiscarded)
            {
                DataFreeFunc(FreeClientData);
                Formats = NULL;
                LocalMetadata = NULL;
                Data = NULL;
            }
        }
        else
        {
            while (Stream->QueuedTimestepCount >= Stream->QueueLimit)
            {
                CP_verbose(Stream,
                           "Provide Timestep for Step %d paused because of "
                           "queueing limit.\n",
                           Timestep);
                pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
                if (Stream->QueuedTimestepCount < Stream->QueueLimit)
                {
                    CP_verbose(Stream, "Provide Timestep for Step %d is now "
                                       "released to continue.\n",
                               Timestep);
                }
            }
        }
    }
    pthread_mutex_unlock(&Stream->DataLock);

    if (Data)
    {
        Stream->DP_Interface->provideTimestep(&Svcs, Stream->DP_Stream, Data,
                                              LocalMetadata, Timestep,
                                              &DP_TimestepInfo);
    }
    Md.RequestGlobalOp =
        Stream->GlobalOpRequired || (Stream->ReadRequestQueue != NULL);
    Md.Formats = Formats;
    Md.Metadata = (SstBlock)LocalMetadata;
    Md.DP_TimestepInfo = DP_TimestepInfo;

    pointers = (MetadataPlusDPInfo *)CP_consolidateDataToAll(
        Stream, &Md, Stream->CPInfo->PerRankMetadataFormat, &data_block);

    for (int i = 0; i < Stream->CohortSize; i++)
    {
        GlobalOpRequested |= pointers[i]->RequestGlobalOp;
    }
    if (GlobalOpRequested)
    {
        DoWriterSideGlobalOp(Stream);
    }

    Msg->CohortSize = Stream->CohortSize;
    Msg->Timestep = Stream->WriterTimestep;

    /* separate metadata and DP_info to separate arrays */
    Msg->Metadata = malloc(Stream->CohortSize * sizeof(void *));
    Msg->DP_TimestepInfo = malloc(Stream->CohortSize * sizeof(void *));
    int NullCount = 0;
    for (int i = 0; i < Stream->CohortSize; i++)
    {
        Msg->Metadata[i] = pointers[i]->Metadata;
        Msg->DP_TimestepInfo[i] = pointers[i]->DP_TimestepInfo;
        if (pointers[i]->DP_TimestepInfo == NULL)
            NullCount++;
        XmitFormats = AddUniqueFormats(XmitFormats, pointers[i]->Formats);
    }
    if (NullCount == Stream->CohortSize)
    {
        free(Msg->DP_TimestepInfo);
        Msg->DP_TimestepInfo = NULL;
    }
    Stream->PreviousFormats =
        AddUniqueFormats(Stream->PreviousFormats, XmitFormats);

    if (Stream->NewReaderPresent)
    {
        /*
         *  If there is a new reader cohort, those ranks will need all prior FFS
         * Format info.
         */
        Msg->Formats = Stream->PreviousFormats;
        Stream->NewReaderPresent = 0;
    }
    else
    {
        Msg->Formats = XmitFormats;
    }

    if (Data == NULL)
    {
        /* Data was actually discarded, but we want to send a message to each
         * reader so that it knows a step was discarded, but actually so that we
         * get an error return if the write fails */

        Msg->Metadata = NULL;
        Msg->DP_TimestepInfo = NULL;
        Msg->Formats = NULL;
        CP_verbose(Stream, "Sending Empty TimestepMetadata for Discarded "
                           "timestep %d, one to each reader\n",
                   Timestep);

        sendOneToEachReaderRank(Stream,
                                Stream->CPInfo->DeliverTimestepMetadataFormat,
                                Msg, &Msg->RS_Stream);
        return;
    }

    /*
     * lock this Stream's data and queue the timestep
     */
    pthread_mutex_lock(&Stream->DataLock);
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        if (Stream->Readers[i]->ReaderStatus == Established)
        {
            /* the metadata will be sent only to established readers */
            Stream->Readers[i]->LastSentTimestep = Timestep;
            Entry->ReferenceCount++;
        }
    }
    Stream->LastProvidedTimestep = Timestep;
    Entry->Data = Data;
    Entry->Timestep = Timestep;
    Entry->Msg = Msg;
    Entry->MetadataArray = Msg->Metadata;
    Entry->DP_TimestepInfo = Msg->DP_TimestepInfo;
    Entry->DataFreeFunc = DataFreeFunc;
    Entry->FreeClientData = FreeClientData;
    Entry->Next = Stream->QueuedTimesteps;
    Stream->QueuedTimesteps = Entry;
    Stream->QueuedTimestepCount++;
    /* no one waits on timesteps being added, so no condition signal to note
     * change */
    pthread_mutex_unlock(&Stream->DataLock);

    /*
     * This barrier deals with a possible race condition on the return
     * of relase timestep messages.  It's possible for one rank to get
     * far enough ahead that it has sent metadata and received a
     * release timestep message before another rank even gets to the
     * point of enqueueing its timestep.  We'll try to eliminate this
     * barrier in another way in the future.
     */
    MPI_Barrier(Stream->mpiComm);

    CP_verbose(Stream, "Sending TimestepMetadata for timestep %d (ref count "
                       "%d), one to each reader\n",
               Timestep, Entry->ReferenceCount);

    sendOneToEachReaderRank(Stream,
                            Stream->CPInfo->DeliverTimestepMetadataFormat, Msg,
                            &Msg->RS_Stream);
}

extern long SstCurrentStep(SstStream Stream) { return Stream->ReaderTimestep; }

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
    char *Filename = TrimSuffix(Name);

    Stream = CP_newStream();
    Stream->Role = ReaderRole;

    CP_validateParams(Stream, Params, 0 /* reader */);

    Stream->DP_Interface = LoadDP("dummy");

    Stream->CPInfo = CP_getCPInfo(Stream->DP_Interface);

    Stream->mpiComm = comm;

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
        sscanf(Writer0Contact, "%p:%s", &WriterFileID, CMContactString);
        //        printf("Writer contact info is fileID %p, contact info %s\n",
        //               WriterFileID, CMContactString);

        attr_list WriterRank0Contact = attr_list_from_string(CMContactString);
        CMConnection conn = CMget_conn(Stream->CPInfo->cm, WriterRank0Contact);
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

        CMwrite(conn, Stream->CPInfo->ReaderRegisterFormat, &ReaderRegister);
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
        struct _CombinedWriterInfo WriterData;
        WriterData.WriterCohortSize = response->WriterCohortSize;
        WriterData.StartingStepNumber = response->NextStepNumber;
        WriterData.CP_WriterInfo = response->CP_WriterInfo;
        WriterData.DP_WriterInfo = response->DP_WriterInfo;
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
    //    printf("I am reader rank %d, my info on writers is:\n", Stream->Rank);
    //    FMdump_data(FMFormat_of_original(Stream->CPInfo->combined_writer_Format),
    //                ReturnData, 1024000);
    //    printf("\n");

    Stream->WriterCohortSize = ReturnData->WriterCohortSize;
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
    return Stream;
}

void queueReaderRegisterMsgAndNotify(SstStream Stream,
                                     struct _ReaderRegisterMsg *Req,
                                     CMConnection conn)
{
    pthread_mutex_lock(&Stream->DataLock);
    RequestQueue New = malloc(sizeof(struct _RequestQueue));
    New->Msg = Req;
    New->Conn = conn;
    New->Next = NULL;
    if (Stream->ReadRequestQueue)
    {
        RequestQueue Last = Stream->ReadRequestQueue;
        while (Last->Next)
        {
            Last = Last->Next;
        }
        Last->Next = New;
    }
    else
    {
        Stream->ReadRequestQueue = New;
    }
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
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

void CP_ReaderRegisterHandler(CManager cm, CMConnection conn, void *Msg_v,
                              void *client_data, attr_list attrs)
{
    SstStream Stream;
    struct _ReaderRegisterMsg *Msg = (struct _ReaderRegisterMsg *)Msg_v;
    //    fprintf(stderr,
    //            "Received a reader registration message directed at writer
    //            %p\n",
    //            Msg->writer_file);
    //    fprintf(stderr, "A reader cohort of size %d is requesting to be
    //    added\n",
    //            Msg->ReaderCohortSize);
    //    for (int i = 0; i < Msg->ReaderCohortSize; i++) {
    //        fprintf(stderr, " rank %d CP contact info: %s, %d, %p\n", i,
    //                Msg->CP_ReaderInfo[i]->ContactInfo,
    //                Msg->CP_ReaderInfo[i]->target_stone,
    //                Msg->CP_ReaderInfo[i]->ReaderID);
    //    }
    Stream = Msg->WriterFile;

    /* arrange for this message data to stay around */
    CMtake_buffer(cm, Msg);

    queueReaderRegisterMsgAndNotify(Stream, Msg, conn);
}

void CP_ReaderActivateHandler(CManager cm, CMConnection conn, void *Msg_v,
                              void *client_data, attr_list attrs)
{
    struct _ReaderActivateMsg *Msg = (struct _ReaderActivateMsg *)Msg_v;

    WS_ReaderInfo CP_WSR_Stream = Msg->WSR_Stream;
    CP_verbose(CP_WSR_Stream->ParentStream, "Reader Activate message received "
                                            "for Stream %p.  Setting state to "
                                            "Established.\n",
               CP_WSR_Stream);
    CP_verbose(CP_WSR_Stream->ParentStream,
               "Parent stream reader count is now %d.\n",
               CP_WSR_Stream->ParentStream->ReaderCount);
    pthread_mutex_lock(&CP_WSR_Stream->ParentStream->DataLock);
    CP_WSR_Stream->ReaderStatus = Established;
    /*
     * the main thread might be waiting for this
     */
    pthread_cond_signal(&CP_WSR_Stream->ParentStream->DataCondition);
    pthread_mutex_unlock(&CP_WSR_Stream->ParentStream->DataLock);
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

extern CPTimestepList dequeueTimestep(SstStream Stream, long Timestep)
{
    CPTimestepList Ret = NULL;
    CPTimestepList List = NULL;
    pthread_mutex_lock(&Stream->DataLock);
    List = Stream->QueuedTimesteps;
    if (Stream->QueuedTimesteps->Timestep == Timestep)
    {
        Stream->QueuedTimesteps = List->Next;
        Ret = List;
    }
    else
    {
        CPTimestepList Last = List;
        List = List->Next;
        while (List != NULL)
        {
            if (List->Timestep == Timestep)
            {
                Last->Next = List->Next;
                Ret = List;
            }
            Last = List;
            List = List->Next;
        }
        if (Ret == NULL)
        {
            /*
             * Shouldn't ever get here because we should never dequeue a
             * timestep that we don't have.
             */
            fprintf(stderr, "Failed to dequeue Timestep %ld, not found\n",
                    Timestep);
            assert(0);
        }
    }
    Stream->QueuedTimestepCount--;
    /* main thread might be waiting on timesteps going away */
    pthread_cond_signal(&Stream->DataCondition);
    pthread_mutex_unlock(&Stream->DataLock);
    return NULL;
}

extern void CP_ReleaseTimestepHandler(CManager cm, CMConnection conn,
                                      void *Msg_v, void *client_data,
                                      attr_list attrs)
{
    struct _ReleaseTimestepMsg *Msg = (struct _ReleaseTimestepMsg *)Msg_v;
    WS_ReaderInfo Reader = (WS_ReaderInfo)Msg->WSR_Stream;
    SstStream Stream = Reader->ParentStream;
    CPTimestepList Entry = NULL;

    CP_verbose(Stream, "Received a release timestep message "
                       "for timestep %d\n",
               Msg->Timestep);

    /* decrement the reference count for the released timestep */
    SubRefRangeTimestep(Stream, Msg->Timestep, Msg->Timestep);
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
        if (Stream->Status != Established)
        {
            pthread_mutex_unlock(&Stream->DataLock);
            CP_verbose(Stream, "Wait for next metadata returning NULL because "
                               "Stream is not Established\n");
            /* closed or failed, return NULL */
            return NULL;
        }
        CP_verbose(Stream,
                   "Waiting for metadata for a Timestep later than TS %d\n",
                   LastTimestep);
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

void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
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
        FFSMarshalInstallMetadata(Stream, Entry->MetadataMsg);
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
    gettimeofday(&CloseTime, NULL);
    timersub(&CloseTime, &Stream->ValidStartTime, &Diff);
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

extern void SstSetStatsSave(SstStream Stream, SstStats Stats)
{
    Stats->OpenTimeSecs = Stream->OpenTimeSecs;
    Stream->Stats = Stats;
}

static void DP_verbose(SstStream s, char *Format, ...)
{
    if (s->Verbose)
    {
        va_list Args;
        va_start(Args, Format);
        if (s->Role == ReaderRole)
        {
            fprintf(stderr, "DP Reader %d (%p): ", s->Rank, s);
        }
        else
        {
            fprintf(stderr, "DP Writer %d (%p): ", s->Rank, s);
        }
        vfprintf(stderr, Format, Args);
        va_end(Args);
    }
}
extern void CP_verbose(SstStream s, char *Format, ...)
{
    if (s->Verbose)
    {
        va_list Args;
        va_start(Args, Format);
        if (s->Role == ReaderRole)
        {
            fprintf(stderr, "Reader %d (%p): ", s->Rank, s);
        }
        else
        {
            fprintf(stderr, "Writer %d (%p): ", s->Rank, s);
        }
        vfprintf(stderr, Format, Args);
        va_end(Args);
    }
}

static CManager CP_getCManager(SstStream Stream) { return Stream->CPInfo->cm; }

static MPI_Comm CP_getMPIComm(SstStream Stream) { return Stream->mpiComm; }

static void CP_sendToPeer(SstStream s, CP_PeerCohort Cohort, int Rank,
                          CMFormat Format, void *Data)
{
    CP_PeerConnection *Peers = (CP_PeerConnection *)Cohort;
    if (Peers[Rank].CMconn == NULL)
    {
        Peers[Rank].CMconn = CMget_conn(s->CPInfo->cm, Peers[Rank].ContactList);
    }
    CMwrite(Peers[Rank].CMconn, Format, Data);
}
