#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "adios2/ADIOSConfig.h"
#include <atl.h>
#include <evpath.h>
#ifdef ADIOS2_HAVE_MPI
#include <mpi.h>
#else
#include "sstmpidummy.h"
#endif
#include <pthread.h>

#include "sst.h"

#include "cp_internal.h"

extern void CP_verbose(SstStream Stream, char *Format, ...);

static void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
                                    void **WS_StreamPtr);
static void CP_PeerFailCloseWSReader(WS_ReaderInfo CP_WSR_Stream,
                                     enum StreamStatus NewState);

#ifdef MUTEX_DEBUG
#define PTHREAD_MUTEX_LOCK(lock)                                               \
    printf("Trying lock line %d\n", __LINE__);                                 \
    pthread_mutex_lock(lock);                                                  \
    printf("Got lock\n");
#define PTHREAD_MUTEX_UNLOCK(lock)                                             \
    printf("UNlocking line %d\n", __LINE__);                                   \
    pthread_mutex_unlock(lock);
#else
#define PTHREAD_MUTEX_LOCK(lock) pthread_mutex_lock(lock);
#define PTHREAD_MUTEX_UNLOCK(lock) pthread_mutex_unlock(lock);
#endif

static char *buildContactInfo(SstStream Stream)
{
    char *Contact = CP_GetContactString(Stream);
    char *FullInfo = malloc(strlen(Contact) + 20);
    sprintf(FullInfo, "%p:%s", (void *)Stream, Contact);
    free(Contact);
    return FullInfo;
}

struct NameListEntry
{
    const char *FileName;
    struct NameListEntry *Next;
};

struct NameListEntry *FileNameList = NULL;

static void RemoveAllFilesInList()
{
    while (FileNameList)
    {
        struct NameListEntry *Next = FileNameList->Next;
        fprintf(stderr, "SST stream open at exit, unlinking contact file %s\n",
                FileNameList->FileName);
        unlink(FileNameList->FileName);
        free(FileNameList);
        FileNameList = Next;
    }
}

static void ExitAndRemoveFiles(int signum)
{
    fprintf(stderr, "ADIOS2 caught SigInt, exiting with error\n");
    exit(1);
}

static void AddNameToExitList(const char *FileName)
{
    static int First = 1;
    if (First)
    {
        struct sigaction sa;
        First = 0;
        atexit(RemoveAllFilesInList);

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = ExitAndRemoveFiles;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
    }

    struct NameListEntry *NewHead = malloc(sizeof(*NewHead));
    NewHead->FileName = FileName;
    NewHead->Next = FileNameList;
    FileNameList = NewHead;
}

static void RemoveNameFromExitList(const char *FileName)
{
    struct NameListEntry **LastPtr = &FileNameList;
    struct NameListEntry *List = FileNameList;
    while (List)
    {
        if (strcmp(FileName, List->FileName) == 0)
        {
            *LastPtr = List->Next;
            free(List);
            return;
        }
        List = List->Next;
    }
}

static void writeContactInfoFile(const char *Name, SstStream Stream)
{
    char *Contact = buildContactInfo(Stream);
    char *TmpName = malloc(strlen(Name) + strlen(".tmp") + 1);
    char *FileName = malloc(strlen(Name) + strlen(SST_POSTFIX) + 1);
    FILE *WriterInfo;

    /*
     * write the contact information file with a temporary name before
     * renaming it to the final version to help prevent partial reads
     */
    sprintf(TmpName, "%s.tmp", Name);
    sprintf(FileName, "%s" SST_POSTFIX, Name);
    WriterInfo = fopen(TmpName, "w");
    fprintf(WriterInfo, "%s", Contact);
    fclose(WriterInfo);
    rename(TmpName, FileName);
    Stream->AbsoluteFilename = realpath(FileName, NULL);
    free(Contact);
    free(TmpName);
    free(FileName);
    AddNameToExitList(Stream->AbsoluteFilename);
}

static void writeContactInfoScreen(const char *Name, SstStream Stream)
{
    char *Contact = buildContactInfo(Stream);

    /*
     * write the contact information file to the screen
     */
    fprintf(stdout, "The next line of output is the contact information "
                    "associated with SST output stream \"%s\".  Please make it "
                    "available to the reader.\n",
            Name);
    fprintf(stdout, "\t%s\n", Contact);
    free(Contact);
}

static void registerContactInfo(const char *Name, SstStream Stream)
{
    switch (Stream->RegistrationMethod)
    {
    case SstRegisterFile:
        writeContactInfoFile(Name, Stream);
        break;
    case SstRegisterScreen:
        writeContactInfoScreen(Name, Stream);
        break;
    case SstRegisterCloud:
        /* not yet */
        break;
    }
}

static void removeContactInfoFile(SstStream Stream)
{
    const char *Name = Stream->AbsoluteFilename;
    unlink(Name);
    RemoveNameFromExitList(Name);
}

static void removeContactInfo(SstStream Stream)
{
    switch (Stream->RegistrationMethod)
    {
    case SstRegisterFile:
        removeContactInfoFile(Stream);
        break;
    case SstRegisterScreen:
        /* nothing necessary here */
        break;
    case SstRegisterCloud:
        /* not yet */
        break;
    }
}

static void WriterConnCloseHandler(CManager cm, CMConnection closed_conn,
                                   void *client_data)
{
    WS_ReaderInfo WSreader = (WS_ReaderInfo)client_data;
    SstStream ParentWriterStream = WSreader->ParentStream;

    PTHREAD_MUTEX_LOCK(&ParentWriterStream->DataLock);
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
        PTHREAD_MUTEX_UNLOCK(&ParentWriterStream->DataLock);
        CP_PeerFailCloseWSReader(WSreader, PeerFailed);
        PTHREAD_MUTEX_LOCK(&ParentWriterStream->DataLock);
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
        PTHREAD_MUTEX_UNLOCK(&ParentWriterStream->DataLock);
        CP_PeerFailCloseWSReader(WSreader, PeerFailed);
        PTHREAD_MUTEX_LOCK(&ParentWriterStream->DataLock);
    }
    /* main thread might be waiting on status change */
    pthread_cond_signal(&ParentWriterStream->DataCondition);
    PTHREAD_MUTEX_UNLOCK(&ParentWriterStream->DataLock);
}

static int initWSReader(WS_ReaderInfo reader, int ReaderSize,
                        CP_ReaderInitInfo *reader_info)
{
    int WriterSize = reader->ParentStream->CohortSize;
    int WriterRank = reader->ParentStream->Rank;
    int i;
    reader->ReaderCohortSize = ReaderSize;
    if (!reader->Connections)
    {
        reader->Connections =
            calloc(sizeof(reader->Connections[0]), ReaderSize);
    }
    for (i = 0; i < ReaderSize; i++)
    {
        if (!reader->Connections[i].ContactList)
        {
            reader->Connections[i].ContactList =
                attr_list_from_string(reader_info[i]->ContactInfo);
        }
        reader->Connections[i].RemoteStreamID = reader_info[i]->ReaderID;
        reader->Connections[i].CMconn = NULL;
    }
    reader->Peers = setupPeerArray(WriterSize, WriterRank, ReaderSize);
    i = 0;
    if (reader->Peers[0] == -1)
    {
        /* we would have no peers.  Contact someone at the CP level anyway so we
         * can hope for failure notification */
        int SoloPeer = WriterRank / ((double)WriterSize / (double)ReaderSize);
        if (SoloPeer >= ReaderSize)
            SoloPeer--;
        reader->Connections[SoloPeer].CMconn =
            CMget_conn(reader->ParentStream->CPInfo->cm,
                       reader->Connections[SoloPeer].ContactList);

        if (!reader->Connections[SoloPeer].CMconn)
        {
            CP_error(
                reader->ParentStream,
                "Connection failed in SstInitWSReader! Contact list was:\n");
            CP_error(
                reader->ParentStream, "%s\n",
                attr_list_to_string(reader->Connections[SoloPeer].ContactList));
            /* fail the stream */
            return 0;
        }

        CMconn_register_close_handler(reader->Connections[SoloPeer].CMconn,
                                      WriterConnCloseHandler, (void *)reader);
    }
    while (reader->Peers[i] != -1)
    {
        int peer = reader->Peers[i];
        reader->Connections[peer].CMconn =
            CMget_conn(reader->ParentStream->CPInfo->cm,
                       reader->Connections[peer].ContactList);

        if (!reader->Connections[peer].CMconn)
        {
            CP_error(
                reader->ParentStream,
                "Connection failed in SstInitWSReader! Contact list was:\n");
            CP_error(
                reader->ParentStream, "%s\n",
                attr_list_to_string(reader->Connections[peer].ContactList));
            /* fail the stream */
            return 0;
        }

        CMconn_register_close_handler(reader->Connections[peer].CMconn,
                                      WriterConnCloseHandler, (void *)reader);
        i++;
    }
    return 1;
}

static long earliestAvailableTimestepNumber(SstStream Stream,
                                            long CurrentTimestep)
{
    long Ret = CurrentTimestep;
    CPTimestepList List = Stream->QueuedTimesteps;
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    while (List)
    {
        if (List->Timestep < Ret)
        {
            List->ReferenceCount++;
            Ret = List->Timestep;
        }
        List = List->Next;
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    return Ret;
}

static void AddRefRangeTimestep(SstStream Stream, long LowRange, long HighRange)
{
    CPTimestepList List;
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    List = Stream->QueuedTimesteps;
    while (List)
    {
        if ((List->Timestep >= LowRange) && (List->Timestep <= HighRange))
        {
            List->ReferenceCount++;
        }
        List = List->Next;
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

static void SubRefRangeTimestep(SstStream Stream, long LowRange, long HighRange)
{
    CPTimestepList Last = NULL, List;
    int AnythingRemoved = 0;
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
                       ItemToFree->Timestep);
            Stream->DP_Interface->releaseTimestep(&Svcs, Stream->DP_Stream,
                                                  List->Timestep);

            Stream->QueuedTimestepCount--;
            List = List->Next;
            ItemToFree->FreeTimestep(ItemToFree->FreeClientData);
            free(ItemToFree->Msg);
            free(ItemToFree->MetadataArray);
            free(ItemToFree->DP_TimestepInfo);
            free(ItemToFree->DataBlockToFree);
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
}

static void KillZeroRefTimesteps(SstStream Stream)
{
    CPTimestepList Last = NULL, List;
    int AnythingRemoved = 0;
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
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
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
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
        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
        assert((Stream->ReadRequestQueue));
        Req = Stream->ReadRequestQueue;
        Stream->ReadRequestQueue = Req->Next;
        Req->Next = NULL;
        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
        struct _CombinedReaderInfo reader_data;
        memset(&reader_data, 0, sizeof(reader_data));
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
    CP_WSR_Stream->ReaderStatus = NotOpen;
    Stream->Readers[Stream->ReaderCount] = CP_WSR_Stream;
    CP_WSR_Stream->DP_WSR_Stream = per_reader_Stream;
    CP_WSR_Stream->ParentStream = Stream;
    CP_WSR_Stream->Connections = connections_to_reader;

    int success = initWSReader(CP_WSR_Stream, ReturnData->ReaderCohortSize,
                               ReturnData->CP_ReaderInfo);

    if (!success)
    {
        return NULL;
    }
    AddToLastCallFreeList(CP_WSR_Stream);
    free(free_block);
    ReturnData = NULL; /* now invalid */

    Stream->ReaderCount++;

    struct _CP_DP_PairInfo combined_init;
    struct _CP_WriterInitInfo cpInfo;

    struct _CP_DP_PairInfo **pointers = NULL;

    cpInfo.ContactInfo = CP_GetContactString(Stream);
    cpInfo.WriterID = CP_WSR_Stream;

    combined_init.CP_Info = (void **)&cpInfo;
    combined_init.DP_Info = DP_WriterInfo;

    MyStartingTimestep =
        earliestAvailableTimestepNumber(Stream, Stream->WriterTimestep);
    if (MyStartingTimestep == -1)
        MyStartingTimestep = 0;

    MPI_Allreduce(&MyStartingTimestep, &GlobalStartingTimestep, 1, MPI_LONG,
                  MPI_MAX, Stream->mpiComm);

    /*
     *  The earliestAvailableTimestepNumber subroutine also added to the
     *  reference count of all existing timesteps, in case we will be making
     *  it available to the joining reader.  Once we've determined what the
     *  starting timestep we'll make available is (GlobalStartingTimestep),
     *  decrement the ref count of others.
     */
    if (MyStartingTimestep != GlobalStartingTimestep)
    {
        SubRefRangeTimestep(Stream, MyStartingTimestep,
                            GlobalStartingTimestep - 1);
    }
    CP_verbose(Stream,
               "My oldest timestep was %ld, global oldest timestep was %ld\n",
               MyStartingTimestep, GlobalStartingTimestep);

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
        response.WriterConfigParams = Stream->ConfigParams;
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
        free(response.CP_WriterInfo);
        free(response.DP_WriterInfo);
    }
    free(cpInfo.ContactInfo);
    if (ret_data_block)
        free(ret_data_block);
    if (pointers)
        free(pointers);
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
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    while (Reader->ReaderStatus != Established)
    {
        /* NEED TO HANDLE FAILURE HERE */
        CP_verbose(Stream, "Waiting for Reader ready on WSR %p.\n", Reader);
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    assert(Reader->ReaderStatus == Established);
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);

    /* send any queued metadata necessary */
    Reader->OldestUnreleasedTimestep = Reader->StartingTimestep;
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

SstStream SstWriterOpen(const char *Name, SstParams Params, MPI_Comm comm)
{
    SstStream Stream;

    Stream = CP_newStream();
    Stream->Role = WriterRole;
    CP_validateParams(Stream, Params, 1 /* Writer */);
    Stream->ConfigParams = Params;

    char *Filename = strdup(Name);
    CP_verbose(Stream, "Loading DataPlane \"%s\"\n", Stream->DataTransport);
    Stream->DP_Interface = LoadDP(Stream->DataTransport);

    if (!Stream->DP_Interface)
    {
        CP_verbose(Stream, "Failed to load DataPlane %s for Stream \"%s\"\n",
                   Stream->DataTransport, Filename);
        return NULL;
    }

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

    Stream->DP_Stream =
        Stream->DP_Interface->initWriter(&Svcs, Stream, Stream->ConfigParams);

    if (Stream->Rank == 0)
    {
        registerContactInfo(Filename, Stream);
    }

    CP_verbose(Stream, "Opening Stream \"%s\"\n", Filename);

    while (Stream->RendezvousReaderCount > 0)
    {
        WS_ReaderInfo reader;
        CP_verbose(Stream, "Stream \"%s\" waiting for %d readers\n", Filename,
                   Stream->RendezvousReaderCount);
        if (Stream->Rank == 0)
        {
            PTHREAD_MUTEX_LOCK(&Stream->DataLock);
            if (Stream->ReadRequestQueue == NULL)
            {
                pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
            }
            assert(Stream->ReadRequestQueue);
            PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
        }
        MPI_Barrier(Stream->mpiComm);

        struct timeval Start, Stop, Diff;
        gettimeofday(&Start, NULL);
        reader = WriterParticipateInReaderOpen(Stream);
        if (!reader)
        {
            CP_error(Stream, "Potential reader registration failed\n");
            break;
        }
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
    AddToLastCallFreeList(Stream);
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

static void CP_PeerFailCloseWSReader(WS_ReaderInfo CP_WSR_Stream,
                                     enum StreamStatus NewState);

static void CloseWSRStream(CManager cm, void *WSR_Stream_v)
{
    WS_ReaderInfo CP_WSR_Stream = (WS_ReaderInfo)WSR_Stream_v;
    CP_PeerFailCloseWSReader(CP_WSR_Stream, PeerClosed);
}

static void CP_PeerFailCloseWSReader(WS_ReaderInfo CP_WSR_Stream,
                                     enum StreamStatus NewState)
{
    SstStream ParentStream = CP_WSR_Stream->ParentStream;
    PTHREAD_MUTEX_LOCK(&ParentStream->DataLock);
    if ((NewState == PeerClosed) || (NewState == Closed))
    {
        SubRefRangeTimestep(CP_WSR_Stream->ParentStream,
                            CP_WSR_Stream->StartingTimestep,
                            CP_WSR_Stream->LastSentTimestep);
    }
    if (NewState == PeerFailed)
    {
        CMadd_delayed_task(ParentStream->CPInfo->cm, 2, 0, CloseWSRStream,
                           CP_WSR_Stream);
    }
    CP_WSR_Stream->ReaderStatus = NewState;
    /* main thread might be waiting on timesteps going away */
    pthread_cond_signal(&ParentStream->DataCondition);
    PTHREAD_MUTEX_UNLOCK(&ParentStream->DataLock);
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
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    while (Stream->QueuedTimesteps)
    {
        CP_verbose(Stream,
                   "Waiting for timesteps to be released in WriterClose\n");
        if (Stream->Verbose)
        {
            CPTimestepList List = Stream->QueuedTimesteps;
            char *StringList = malloc(1);
            StringList[0] = 0;
            while (List)
            {
                char tmp[20];
                sprintf(tmp, "%ld ", List->Timestep);
                StringList =
                    realloc(StringList, strlen(StringList) + strlen(tmp) + 1);
                strcat(StringList, tmp);
                List = List->Next;
            }
            CP_verbose(Stream, "The timesteps still queued are: %s\n",
                       StringList);
            free(StringList);
        }
        CP_verbose(Stream, "Reader Count is %d\n", Stream->ReaderCount);
        for (int i = 0; i < Stream->ReaderCount; i++)
        {
            printf("Reader [%d] status is %d\n", i,
                   Stream->Readers[i]->ReaderStatus);
        }
        /* NEED TO HANDLE FAILURE HERE */
        pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);

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

#ifdef NOTDEF
static FFSFormatList AddUniqueFormats(FFSFormatList List,
                                      FFSFormatList Candidates)
{
    while (Candidates)
    {
        FFSFormatList Tmp = List;
        int found = 0;
        while (Tmp)
        {
            if ((Tmp->FormatIDRepLen == Candidates->FormatIDRepLen) &&
                (memcmp(Tmp->FormatIDRep, Candidates->FormatIDRep,
                        Tmp->FormatIDRepLen) == 0))
            {
                found++;
                break;
            }
            Tmp = Tmp->Next;
        }
        if (!found)
        {
            FFSFormatList New = malloc(sizeof(*New));
            memset(New, 0, sizeof(*New));
            New->FormatServerRep = malloc(Candidates->FormatServerRepLen);
            memcpy(New->FormatServerRep, Candidates->FormatServerRep,
                   Candidates->FormatServerRepLen);
            New->FormatServerRepLen = Candidates->FormatServerRepLen;
            New->FormatIDRep = malloc(Candidates->FormatIDRepLen);
            memcpy(New->FormatIDRep, Candidates->FormatIDRep,
                   Candidates->FormatIDRepLen);
            New->FormatIDRepLen = Candidates->FormatIDRepLen;
            New->Next = List;
            List = New;
        }
        Candidates = Candidates->Next;
    }
    return List;
}
#endif

static FFSFormatList AddUniqueFormats(FFSFormatList List,
                                      FFSFormatList Candidates, int copy)
{
    FFSFormatList Tmp = List;
    FFSFormatList Ret = List;

    // If nothing to add, return original
    if (!Candidates)
        return Ret;

    // Add tail of candidates list first
    Ret = AddUniqueFormats(List, Candidates->Next, copy);

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
    if (copy)
    {
        // Copy top Candidates entry before return
        FFSFormatList Tmp = malloc(sizeof(*Tmp));
        memset(Tmp, 0, sizeof(*Tmp));
        Tmp->FormatServerRep = malloc(Candidates->FormatServerRepLen);
        memcpy(Tmp->FormatServerRep, Candidates->FormatServerRep,
               Candidates->FormatServerRepLen);
        Tmp->FormatServerRepLen = Candidates->FormatServerRepLen;
        Tmp->FormatIDRep = malloc(Candidates->FormatIDRepLen);
        memcpy(Tmp->FormatIDRep, Candidates->FormatIDRep,
               Candidates->FormatIDRepLen);
        Tmp->FormatIDRepLen = Candidates->FormatIDRepLen;
        Candidates = Tmp;
    }
    Candidates->Next = Ret;
    return Candidates;
}

static void DoStreamDiscard(SstStream Stream)
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
        Entry->FreeTimestep(Entry->FreeClientData);
        free(Entry->Msg);
        free(Entry->DataBlockToFree);
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
        Entry->FreeTimestep(Entry->FreeClientData);
        free(Entry->MetadataArray);
        free(Entry->DP_TimestepInfo);
        free(Entry->DataBlockToFree);
        free(Entry->Msg->Metadata);
        if (Entry->Msg->DP_TimestepInfo)
            free(Entry->Msg->DP_TimestepInfo);
        free(Entry->Msg);
        free(Entry->DataBlockToFree);
        free(Entry);
        Stream->QueuedTimestepCount--;
    }
}

static void DoWriterSideGlobalOp(SstStream Stream, int *DiscardIncomingTimestep)
{
    int SendBlockSize = Stream->ReaderCount + 2;
    int *SendBlock = malloc(sizeof(SendBlock[0]) * SendBlockSize);
    int *RecvBlock =
        malloc(sizeof(SendBlock[0]) * SendBlockSize * Stream->CohortSize);

    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    CP_verbose(Stream, "Initiating Writer Side global operation\n");
    Stream->GlobalOpRequired = 0; //  reset if set
    RequestQueue ArrivingReader = Stream->ReadRequestQueue;

    /* build local info item -- contents:
     *  - Reader_requests_join (integer, contains count, only rank 0 should ever
     *set)
     *  - Current event queue length for this rank
     *	- Reader_status (integer, repeated ReaderCount times)
     */
    SendBlock[0] = 0;
    while (ArrivingReader)
    {
        SendBlock[0]++;
        ArrivingReader = ArrivingReader->Next;
    }
    if (Stream->QueueLimit &&
        (Stream->QueuedTimestepCount > Stream->QueueLimit))
    {
        SendBlock[1] = 1; /* this rank is over stream limit */
    }
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        SendBlock[i + 2] = Stream->Readers[i]->ReaderStatus;
    }
    MPI_Allgather(SendBlock, SendBlockSize, MPI_INT, RecvBlock, SendBlockSize,
                  MPI_INT, Stream->mpiComm);

    /*
     *  Handle incoming data in a deterministic way.  First handle any changes
     * in state for existing clients.
     */
    int ActiveReaderCount = 0;
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
                if (RecvBlock[j * SendBlockSize + i + 2] == PeerFailed)
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
            PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
            CP_PeerFailCloseWSReader(Stream->Readers[i], Closed);
            PTHREAD_MUTEX_LOCK(&Stream->DataLock);
        }
        if (Stream->Readers[i]->ReaderStatus == Established)
            ActiveReaderCount++;
    }

    /*
     *  Then handle possible incoming connection requests.  (Only rank 0 has
     * valid info.)
     */
    int OverLimit = 0;
    for (int i = 0; i < Stream->CohortSize; i++)
    {
        if (RecvBlock[i * SendBlockSize + 1] > 0)
            OverLimit++;
    }
    /* we've got all the state we need to know, release data lock */
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);

    for (int i = 0; i < RecvBlock[0]; i++)
    {
        WS_ReaderInfo reader;
        CP_verbose(Stream,
                   "Writer side Global operation accepting incoming reader\n");
        reader = WriterParticipateInReaderOpen(Stream);
        if (!reader)
        {
            CP_error(Stream, "Potential reader registration failed\n");
            break;
        }
        waitForReaderResponseAndSendQueued(reader);
    }

    /*
     *  Lastly, we'll decide what to do with the current provided timestep.
     *  If any writer rank is over the queuelimit, we must discard or block
     * decision points:
     If mode is block on queue limit:
         If there are no currently active readers, every node must block on
     reader arrival.
         else if there are active readers, each rank can block on queue size
     decreasing (as it will with progress)
     if mode is discard:
          If there are no currently active readers, we can discard the oldest
     item in the queue
          else if there are active readers, we must discard the currently
     arriving timestep.
     */
    if (OverLimit)
    {
        if (Stream->QueueFullPolicy == SstQueueFullBlock)
        {
            if (ActiveReaderCount == 0)
            {
                while (ActiveReaderCount == 0)
                {
                    WS_ReaderInfo reader;
                    if (Stream->Rank == 0)
                    {
                        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
                        while (Stream->ReadRequestQueue == NULL)
                        {
                            pthread_cond_wait(&Stream->DataCondition,
                                              &Stream->DataLock);
                        }
                        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
                    }
                    MPI_Barrier(Stream->mpiComm);
                    CP_verbose(Stream, "Writer side Global operation blocking "
                                       "for incoming reader\n");
                    reader = WriterParticipateInReaderOpen(Stream);
                    if (!reader)
                    {
                        CP_error(Stream,
                                 "Potential reader registration failed\n");
                        break;
                    }
                    waitForReaderResponseAndSendQueued(reader);
                    ActiveReaderCount++;
                }
            }
            else
            {
                /* wait until enough queued data is sent */
                PTHREAD_MUTEX_LOCK(&Stream->DataLock);
                while (Stream->QueueLimit &&
                       (Stream->QueuedTimestepCount > Stream->QueueLimit))
                {
                    CP_verbose(
                        Stream,
                        "Waiting for timesteps to be released in GlobalOp\n");
                    CP_verbose(Stream,
                               "The first timestep still queued is %d\n",
                               Stream->QueuedTimesteps->Timestep);
                    /* NEED TO HANDLE FAILURE HERE */
                    pthread_cond_wait(&Stream->DataCondition,
                                      &Stream->DataLock);
                }
                PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
            }
        }
        else
        {
            /* discard things */
            if (ActiveReaderCount)
            {
                DoStreamDiscard(Stream);
            }
            else
            {
                *DiscardIncomingTimestep = 1;
            }
        }
    }
    free(SendBlock);
    free(RecvBlock);
    CP_verbose(Stream, "Finished with Writer side Global operations\n");
}

extern void SstInternalProvideTimestep(SstStream Stream, SstData LocalMetadata,
                                       SstData Data, long Timestep,
                                       FFSFormatList Formats,
                                       DataFreeFunc FreeTimestep,
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
    int GlobalOpRequired = 0;

    memset(Msg, 0, sizeof(*Msg));
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    Stream->WriterTimestep = Timestep;
    if ((Stream->QueueLimit > 0) &&
        (Stream->QueuedTimestepCount >= Stream->QueueLimit))
    {
        CP_verbose(Stream, "Requesting global op because of queue limit\n");
        GlobalOpRequested = 1;
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    GlobalOpRequested |= Stream->GlobalOpRequired;
    if (Stream->ReadRequestQueue != NULL)
    {
        CP_verbose(Stream, "Requesting global op because of pending reader\n");
    }
    GlobalOpRequested |=
        (Stream->ReadRequestQueue != NULL); // only non Null on rank 0

    MPI_Allreduce(&GlobalOpRequested, &GlobalOpRequired, 1, MPI_INT, MPI_LOR,
                  Stream->mpiComm);

    if (GlobalOpRequired)
    {
        int IncomingTimestepDiscarded = 0;
        CP_verbose(Stream, "Doing Global Op\n");
        DoWriterSideGlobalOp(Stream, &IncomingTimestepDiscarded);
        if (IncomingTimestepDiscarded)
        {
            CP_verbose(Stream, "Discarding incoming timestep %d because of "
                               "queue full condition\n",
                       Timestep);
            FreeTimestep(FreeClientData);
            Formats = NULL;
            LocalMetadata = NULL;
            Data = NULL;
        }
    }

    if (Data)
    {
        Stream->DP_Interface->provideTimestep(&Svcs, Stream->DP_Stream, Data,
                                              LocalMetadata, Timestep,
                                              &DP_TimestepInfo);
    }

    Md.Formats = Formats;
    Md.Metadata = (SstData)LocalMetadata;
    Md.DP_TimestepInfo = DP_TimestepInfo;

    pointers = (MetadataPlusDPInfo *)CP_consolidateDataToAll(
        Stream, &Md, Stream->CPInfo->PerRankMetadataFormat, &data_block);

    Msg->CohortSize = Stream->CohortSize;
    Msg->Timestep = Timestep;

    /* separate metadata and DP_info to separate arrays */
    Msg->Metadata = malloc(Stream->CohortSize * sizeof(SstData));
    Msg->DP_TimestepInfo =
        malloc(Stream->CohortSize * sizeof(Msg->DP_TimestepInfo[0]));
    int NullCount = 0;
    for (int i = 0; i < Stream->CohortSize; i++)
    {
        Msg->Metadata[i] = pointers[i]->Metadata;
        Msg->DP_TimestepInfo[i] = pointers[i]->DP_TimestepInfo;
        if (pointers[i]->DP_TimestepInfo == NULL)
            NullCount++;
        XmitFormats =
            AddUniqueFormats(XmitFormats, pointers[i]->Formats, /*nocopy*/ 0);
    }
    if (NullCount == Stream->CohortSize)
    {
        free(Msg->DP_TimestepInfo);
        Msg->DP_TimestepInfo = NULL;
    }
    free(pointers);

    Stream->PreviousFormats =
        AddUniqueFormats(Stream->PreviousFormats, XmitFormats, /*copy*/ 1);

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
        free(Entry);
        free(Msg);
        free(data_block);
        return;
    }

    /*
     * lock this Stream's data and queue the timestep
     */
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
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
    Entry->Timestep = Timestep;
    Entry->Msg = Msg;
    Entry->MetadataArray = Msg->Metadata;
    Entry->DP_TimestepInfo = Msg->DP_TimestepInfo;
    Entry->FreeTimestep = FreeTimestep;
    Entry->FreeClientData = FreeClientData;
    Entry->Next = Stream->QueuedTimesteps;
    Entry->DataBlockToFree = data_block;
    Stream->QueuedTimesteps = Entry;
    Stream->QueuedTimestepCount++;
    /* no one waits on timesteps being added, so no condition signal to note
     * change */

    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);

    CP_verbose(Stream, "Sending TimestepMetadata for timestep %d (ref count "
                       "%d), one to each reader\n",
               Timestep, Entry->ReferenceCount);

    /*
     * This barrier deals with a possible race condition on the return
     * of relase timestep messages.  It's possible for one rank to get
     * far enough ahead that it has sent metadata and received a
     * release timestep message before another rank even gets to the
     * point of enqueueing its timestep.  We'll try to eliminate this
     * barrier in another way in the future.
     */
    MPI_Barrier(Stream->mpiComm);

    sendOneToEachReaderRank(Stream,
                            Stream->CPInfo->DeliverTimestepMetadataFormat, Msg,
                            &Msg->RS_Stream);
}

extern void SstProvideTimestep(SstStream Stream, SstData LocalMetadata,
                               SstData Data, long Timestep,
                               DataFreeFunc FreeTimestep, void *FreeClientData)
{

    SstInternalProvideTimestep(Stream, LocalMetadata, Data, Timestep, NULL,
                               FreeTimestep, FreeClientData);
}

void queueReaderRegisterMsgAndNotify(SstStream Stream,
                                     struct _ReaderRegisterMsg *Req,
                                     CMConnection conn)
{
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
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
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

void CP_ReaderCloseHandler(CManager cm, CMConnection conn, void *Msg_v,
                           void *client_data, attr_list attrs)
{
    struct _ReaderCloseMsg *Msg = (struct _ReaderCloseMsg *)Msg_v;
    WS_ReaderInfo CP_WSR_Stream = Msg->WSR_Stream;

    CP_verbose(CP_WSR_Stream->ParentStream,
               "Reader Close message received for stream %p.  Setting state to "
               "PeerClosed and releasing timesteps.\n",
               CP_WSR_Stream);
    CP_PeerFailCloseWSReader(CP_WSR_Stream, PeerClosed);
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
    PTHREAD_MUTEX_LOCK(&CP_WSR_Stream->ParentStream->DataLock);
    CP_WSR_Stream->ReaderStatus = Established;
    /*
     * the main thread might be waiting for this
     */
    pthread_cond_signal(&CP_WSR_Stream->ParentStream->DataCondition);
    PTHREAD_MUTEX_UNLOCK(&CP_WSR_Stream->ParentStream->DataLock);
}

extern CPTimestepList dequeueTimestep(SstStream Stream, long Timestep)
{
    CPTimestepList Ret = NULL;
    CPTimestepList List;
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
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
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    return NULL;
}

extern void CP_ReleaseTimestepHandler(CManager cm, CMConnection conn,
                                      void *Msg_v, void *client_data,
                                      attr_list attrs)
{
    struct _ReleaseTimestepMsg *Msg = (struct _ReleaseTimestepMsg *)Msg_v;
    WS_ReaderInfo Reader = (WS_ReaderInfo)Msg->WSR_Stream;
    SstStream ParentStream = Reader->ParentStream;
    CPTimestepList Entry = NULL;

    CP_verbose(ParentStream, "Received a release timestep message "
                             "for timestep %d\n",
               Msg->Timestep);

    /* decrement the reference count for the released timestep */
    PTHREAD_MUTEX_LOCK(&ParentStream->DataLock);
    SubRefRangeTimestep(ParentStream, Msg->Timestep, Msg->Timestep);
    Reader->OldestUnreleasedTimestep = Msg->Timestep++;
    pthread_cond_signal(&ParentStream->DataCondition);
    PTHREAD_MUTEX_UNLOCK(&ParentStream->DataLock);
}
