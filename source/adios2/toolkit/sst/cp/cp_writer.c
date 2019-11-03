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

#include "adios2/common/ADIOSConfig.h"
#include <atl.h>
#include <evpath.h>
#include <pthread.h>

#include "sst.h"

#include "adios2/toolkit/profiling/taustubs/taustubs.h"
#include "cp_internal.h"

extern void CP_verbose(SstStream Stream, char *Format, ...);

static void sendOneToEachWriterRank(SstStream s, CMFormat f, void *Msg,
                                    void **WS_StreamPtr);
static void CP_PeerFailCloseWSReader(WS_ReaderInfo CP_WSR_Stream,
                                     enum StreamStatus NewState);

static int locked = 0;
#ifdef MUTEX_DEBUG
#define PTHREAD_MUTEX_LOCK(lock)                                               \
    printf("Trying lock line %d\n", __LINE__);                                 \
    pthread_mutex_lock(lock);                                                  \
    locked++;                                                                  \
    printf("Got lock\n");
#define PTHREAD_MUTEX_UNLOCK(lock)                                             \
    printf("UNlocking line %d\n", __LINE__);                                   \
    locked--;                                                                  \
    pthread_mutex_unlock(lock);
#define SST_ASSERT_LOCKED() assert(locked)
#else
#define PTHREAD_MUTEX_LOCK(lock)                                               \
    {                                                                          \
        pthread_mutex_lock(lock);                                              \
        locked++;                                                              \
    }
#define PTHREAD_MUTEX_UNLOCK(lock)                                             \
    {                                                                          \
        locked--;                                                              \
        pthread_mutex_unlock(lock);                                            \
    }
#define SST_ASSERT_LOCKED() assert(locked)
#define SST_ASSERT_UNLOCKED() assert(unlocked)
#endif

static char *buildContactInfo(SstStream Stream, attr_list DPAttrs)
{
    char *Contact = CP_GetContactString(Stream, DPAttrs);
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

static void writeContactInfoFile(const char *Name, SstStream Stream,
                                 attr_list DPAttrs)
{
    char *Contact = buildContactInfo(Stream, DPAttrs);
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
    fprintf(WriterInfo, "%s", SSTMAGICV0);
    fprintf(WriterInfo, "%s", Contact);
    fclose(WriterInfo);
    rename(TmpName, FileName);
    Stream->AbsoluteFilename = realpath(FileName, NULL);
    free(Contact);
    free(TmpName);
    free(FileName);
    AddNameToExitList(Stream->AbsoluteFilename);
}

static void writeContactInfoScreen(const char *Name, SstStream Stream,
                                   attr_list DPAttrs)
{
    char *Contact = buildContactInfo(Stream, DPAttrs);

    /*
     * write the contact information file to the screen
     */
    fprintf(stdout,
            "The next line of output is the contact information "
            "associated with SST output stream \"%s\".  Please make it "
            "available to the reader.\n",
            Name);
    fprintf(stdout, "\t%s\n", Contact);
    free(Contact);
}

static void registerContactInfo(const char *Name, SstStream Stream,
                                attr_list DPAttrs)
{
    switch (Stream->RegistrationMethod)
    {
    case SstRegisterFile:
        writeContactInfoFile(Name, Stream, DPAttrs);
        break;
    case SstRegisterScreen:
        writeContactInfoScreen(Name, Stream, DPAttrs);
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

/*
RemoveQueueEntries:
        If the number of timesteps older than OldestCurrentReaderTimestep, mark
them as Expired Dequeue and free any timestep that is Expired, not Precious and
has reference count 0. if change SIGNAL

*/
static void RemoveQueueEntries(SstStream Stream)
{
    int AnythingRemoved = 0;
    CPTimestepList List = Stream->QueuedTimesteps;
    CPTimestepList Last = NULL;
    CPTimestepList Prev = NULL;

    while (List)
    {
        CPTimestepList Next = List->Next;
        int Freed = 0;
        if (List->Expired && (!List->PreciousTimestep) &&
            (List->ReferenceCount == 0))
        {
            CPTimestepList ItemToFree = List;
            Freed = 1;
            if (ItemToFree->DPRegistered)
            {
                Stream->DP_Interface->releaseTimestep(&Svcs, Stream->DP_Stream,
                                                      List->Timestep);
            }

            Stream->QueuedTimestepCount--;
            CP_verbose(Stream,
                       "Remove queue Entries removing Timestep %ld (exp %d, "
                       "Prec %d, Ref %d), Count now %d\n",
                       ItemToFree->Timestep, ItemToFree->Expired,
                       ItemToFree->PreciousTimestep, ItemToFree->ReferenceCount,
                       Stream->QueuedTimestepCount);
            ItemToFree->FreeTimestep(ItemToFree->FreeClientData);
            free(ItemToFree->Msg);
            //            free(ItemToFree->MetadataArray);
            //            free(ItemToFree->DP_TimestepInfo);
            free(ItemToFree->DataBlockToFree);
            free(ItemToFree);
            AnythingRemoved++;

            if (Last)
            {
                /* unlink item */
                Last->Next = Next;
            }
            else
            {
                Stream->QueuedTimesteps = Next;
            }
        }
        if (!Freed)
        {
            Last = List;
        }
        List = Next;
    }

    if (AnythingRemoved)
    {
        /* main thread might be waiting on timesteps going away */
        pthread_cond_signal(&Stream->DataCondition);
    }
}

static void ReleaseAndDiscardRemainingTimesteps(SstStream Stream)
{
    CPTimestepList List = Stream->QueuedTimesteps;

    while (List)
    {
        List->Expired = 1;
        List->PreciousTimestep = 0;
        List->ReferenceCount = 0;
        List = List->Next;
    }
    RemoveQueueEntries(Stream);
}

/*
Queue maintenance:    (ASSUME LOCKED)
        calculate smallest entry for CurrentTimestep in a reader.  Update that
as OldestCurrentReaderTimestep. If any timestep has zero ref count and is
registered with DP deregister that timestep with DP CallRemoveQueueEntries
*/
static void QueueMaintenance(SstStream Stream)
{
    SST_ASSERT_LOCKED();
    long SmallestLastReleasedTimestep = LONG_MAX;
    long ReserveCount;

    if (Stream->Status != Established)
        return;

    ReserveCount = Stream->ConfigParams->ReserveQueueLimit;
    CPTimestepList List;
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        CP_verbose(Stream,
                   "Reader %d status %s has last released %ld, last sent %ld\n",
                   i, SSTStreamStatusStr[Stream->Readers[i]->ReaderStatus],
                   Stream->Readers[i]->LastReleasedTimestep,
                   Stream->Readers[i]->LastSentTimestep);
        if (Stream->Readers[i]->ReaderStatus == Established)
        {
            if (Stream->Readers[i]->LastReleasedTimestep <
                SmallestLastReleasedTimestep)
                SmallestLastReleasedTimestep =
                    Stream->Readers[i]->LastReleasedTimestep;
        }
    }
    if (SmallestLastReleasedTimestep != LONG_MAX)
    {
        CP_verbose(
            Stream,
            "QueueMaintenance, smallest last released = %ld, count = %d\n",
            SmallestLastReleasedTimestep, Stream->QueuedTimestepCount);
    }
    else
    {
        CP_verbose(
            Stream,
            "QueueMaintenance, smallest last released = LONG_MAX, count = %d\n",
            Stream->QueuedTimestepCount);
    }
    /* Count precious */
    List = Stream->QueuedTimesteps;
    while (List)
    {

        if (List->PreciousTimestep && (List->ReferenceCount == 0))
        {
            /* unreferenced precious timesteps are reserve */
            ReserveCount--;
        }
        List = List->Next;
    }

    List = Stream->QueuedTimesteps;
    while (List)
    {
        if (List->Timestep <= SmallestLastReleasedTimestep)
        {
            ReserveCount--;
            if (ReserveCount < 0)
            {
                if (List->Expired == 0)
                {
                    CP_verbose(Stream,
                               "Writer tagging timestep %ld as expired\n",
                               List->Timestep);
                }
                List->Expired = 1;
                if ((List->ReferenceCount == 0) && (List->DPRegistered) &&
                    (!List->PreciousTimestep))
                {
                    /* unregister with DP */
                    Stream->DP_Interface->releaseTimestep(
                        &Svcs, Stream->DP_Stream, List->Timestep);
                    List->DPRegistered = 0;
                }
            }
        }
        List = List->Next;
    }
    RemoveQueueEntries(Stream);
}

/*
        Identify reader
        LOCK
        decrement reference count on timesteps between LastReleased and LastSent
        LastSent = -1; LastReleased = -1;
        QueueMaintenance
        UNLOCK
*/
extern void WriterConnCloseHandler(CManager cm, CMConnection closed_conn,
                                   void *client_data)
{
    TAU_START_FUNC();
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
        CP_PeerFailCloseWSReader(WSreader, PeerFailed);
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
        CP_verbose(ParentWriterStream,
                   "Writer-side Rank received a "
                   "connection-close event in unexpected "
                   "state %s\n",
                   SSTStreamStatusStr[WSreader->ReaderStatus]);
    }
    QueueMaintenance(ParentWriterStream);
    PTHREAD_MUTEX_UNLOCK(&ParentWriterStream->DataLock);
    TAU_STOP_FUNC();
}

static void SendPeerSetupMsg(WS_ReaderInfo reader, int reversePeer, int myRank)
{
    CMConnection conn = reader->Connections[reversePeer].CMconn;
    SstStream Stream = reader->ParentStream;
    struct _PeerSetupMsg setup;
    setup.RS_Stream = reader->Connections[reversePeer].RemoteStreamID;
    setup.WriterRank = myRank;
    setup.WriterCohortSize = Stream->CohortSize;
    if (CMwrite(conn, Stream->CPInfo->PeerSetupFormat, &setup) != 1)
    {
        CP_verbose(Stream,
                   "Message failed to send to reader in sendPeerSetup in "
                   "reader open\n");
    }
}

static int initWSReader(WS_ReaderInfo reader, int ReaderSize,
                        CP_ReaderInitInfo *reader_info)
{
    SstStream Stream = reader->ParentStream;
    int WriterSize = reader->ParentStream->CohortSize;
    int WriterRank = reader->ParentStream->Rank;
    int i;
    int *reverseArray;
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
    if (Stream->ConfigParams->CPCommPattern == SstCPCommPeer)
    {
        /*
         *   Peering.
         *   We use peering for two things:
         *     - failure awareness (each rank needs a close handler on one
         connection to some opposite rank so they can detect failure)
         *     - notification (how info gets sent from reader to writer and vice
         versa)
         *
         *   A connection that exists for notification is also useful for
         *   failure awareness, but where not are necessary for
         *   notification, we still may make some for failure
         *   notification.

         *   In this code, all connections are made by the writing side,
         *   but the reader side must be sent notifications so that it is
         *   aware of what connections are made for it and what they are
         *   to be used for (I.E. notification, or only existing passively
         *   for failure notification).
         *
         *   Connections that are used for notification from writer to
         *   reader will be in the Peer list and we'll send messages down
         *   them later.  If there are many more writers than readers
         *   (presumed normal case), the peer list will have 0 or 1
         *   entries.  Connections in the reverseArray are for failure
         *   awareness and/or notification from reader to writer.  If
         *   there are many more readers than writers, the reverseArray
         *   will have one entry (to the one reader that will send us
         *   notifications and which we will use for failure awareness).

         *   If there are equal numbers of readers and writers, then each
         *   rank is peered only with the same rank in the opposing.

         *   If there happen to be many more readers than writers, then
         *   the Peer list will contain a lot of entries (all those that
         *   get notifications from us.  The reverseArray will also
         *   contain a lot of entries, but only the first will send us
         *   notifications.  The others will just use the connections for
         *   failure awareness.

         */
        getPeerArrays(WriterSize, WriterRank, ReaderSize, &reader->Peers,
                      &reverseArray);

        i = 0;
        while (reverseArray[i] != -1)
        {
            int peer = reverseArray[i];
            if (reader->ParentStream->ConnectionUsleepMultiplier != 0)
                usleep(WriterRank *
                       reader->ParentStream->ConnectionUsleepMultiplier);
            reader->Connections[peer].CMconn =
                CMget_conn(reader->ParentStream->CPInfo->cm,
                           reader->Connections[peer].ContactList);

            if (!reader->Connections[peer].CMconn)
            {
                CP_error(reader->ParentStream, "Connection failed in "
                                               "SstInitWSReader! Contact list "
                                               "was:\n");
                CP_error(
                    reader->ParentStream, "%s\n",
                    attr_list_to_string(reader->Connections[peer].ContactList));
                /* fail the stream */
                return 0;
            }

            CP_verbose(
                Stream,
                "Registering a close handler for connection %p, to peer %d\n",
                reader->Connections[peer].CMconn, peer);
            CMconn_register_close_handler(reader->Connections[peer].CMconn,
                                          WriterConnCloseHandler,
                                          (void *)reader);
            if (i == 0)
            {
                /* failure awareness for reader rank */
                CP_verbose(reader->ParentStream,
                           "Sending peer setup to rank %d\n", peer);
                SendPeerSetupMsg(reader, peer, reader->ParentStream->Rank);
            }
            else
            {
                CP_verbose(reader->ParentStream,
                           "Sending peer setup to rank %d\n", peer);
                /* failure awareness for reader rank */
                SendPeerSetupMsg(reader, peer, -1);
            }
            i++;
        }
        free(reverseArray);
        i = 0;
        while (reader->Peers[i] != -1)
        {
            int peer = reader->Peers[i];
            if (reader->Connections[peer].CMconn)
            {
                /* already made this above */
                i++;
                continue;
            }
            if (reader->ParentStream->ConnectionUsleepMultiplier != 0)
                usleep(WriterRank *
                       reader->ParentStream->ConnectionUsleepMultiplier);
            reader->Connections[peer].CMconn =
                CMget_conn(reader->ParentStream->CPInfo->cm,
                           reader->Connections[peer].ContactList);

            if (!reader->Connections[peer].CMconn)
            {
                CP_error(reader->ParentStream, "Connection failed in "
                                               "SstInitWSReader! Contact list "
                                               "was:\n");
                CP_error(
                    reader->ParentStream, "%s\n",
                    attr_list_to_string(reader->Connections[peer].ContactList));
                /* fail the stream */
                return 0;
            }

            CMconn_register_close_handler(reader->Connections[peer].CMconn,
                                          WriterConnCloseHandler,
                                          (void *)reader);
            /* failure awareness for reader rank */
            CP_verbose(reader->ParentStream, "Sending peer setup to rank %d\n",
                       peer);
            SendPeerSetupMsg(reader, peer, reader->ParentStream->Rank);
            i++;
        }
    }
    else
    {
        /* Comm Minimum pattern only Writer rank 0 initiates a connection to
         * Reader Peers */
        if (Stream->Rank == 0)
        {
            reader->Connections[0].CMconn =
                CMget_conn(reader->ParentStream->CPInfo->cm,
                           reader->Connections[0].ContactList);

            if (!reader->Connections[0].CMconn)
            {
                CP_error(reader->ParentStream, "Connection failed in "
                                               "SstInitWSReader! Contact list "
                                               "was:\n");
                CP_error(
                    reader->ParentStream, "%s\n",
                    attr_list_to_string(reader->Connections[0].ContactList));
                /* fail the stream */
                return 0;
            }

            CMconn_register_close_handler(reader->Connections[0].CMconn,
                                          WriterConnCloseHandler,
                                          (void *)reader);
        }
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
        CP_verbose(Stream,
                   "Earliest available : Writer-side Timestep %ld "
                   "now has reference count %d, expired %d, precious %d\n",
                   List->Timestep, List->ReferenceCount, List->Expired,
                   List->PreciousTimestep);
        if (List->Timestep < Ret)
        {
            Ret = List->Timestep;
        }
        List = List->Next;
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    return Ret;
}

static void UntagPreciousTimesteps(SstStream Stream)
{
    CPTimestepList Last = NULL, List;
    SST_ASSERT_LOCKED();
    List = Stream->QueuedTimesteps;
    while (List)
    {
        if (List->PreciousTimestep)
        {
            CP_verbose(Stream,
                       "Precious Timestep %d untagged, reference count is %d\n",
                       List->Timestep, List->ReferenceCount);
            List->PreciousTimestep = 0;
            List->Expired = 1;
        }
        List = List->Next;
    }
}

static void SubRefTimestep(SstStream Stream, long Timestep, int SetLast)
{
    CPTimestepList Last = NULL, List;
    int AnythingRemoved = 0;
    List = Stream->QueuedTimesteps;
    SST_ASSERT_LOCKED();
    while (List)
    {
        if (List->Timestep == Timestep)
        {
            List->ReferenceCount--;
            CP_verbose(Stream,
                       "SubRef : Writer-side Timestep %ld "
                       "now has reference count %d, expired %d, precious %d\n",
                       List->Timestep, List->ReferenceCount, List->Expired,
                       List->PreciousTimestep);
        }
        List = List->Next;
    }
}

WS_ReaderInfo WriterParticipateInReaderOpen(SstStream Stream)
{
    RequestQueue Req;
    reader_data_t ReturnData;
    void *free_block = NULL;
    int WriterResponseCondition = -1;
    CMConnection conn;
    long MyStartingTimestep, GlobalStartingTimestep;
    WS_ReaderInfo CP_WSR_Stream = malloc(sizeof(*CP_WSR_Stream));

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
        reader_data.RankZeroID = CP_WSR_Stream;
        reader_data.SpecPreload = Req->Msg->SpecPreload;
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

    memset(CP_WSR_Stream, 0, sizeof(*CP_WSR_Stream));
    CP_WSR_Stream->ReaderStatus = NotOpen;
    CP_WSR_Stream->RankZeroID = ReturnData->RankZeroID;
    Stream->Readers[Stream->ReaderCount] = CP_WSR_Stream;
    CP_WSR_Stream->DP_WSR_Stream = per_reader_Stream;
    CP_WSR_Stream->ParentStream = Stream;
    CP_WSR_Stream->LastReleasedTimestep = -1;
    CP_WSR_Stream->Connections = connections_to_reader;
    CP_WSR_Stream->ReaderDefinitionsLocked = 0;
    CP_WSR_Stream->ReaderSelectionLockTimestep = -1;
    if (ReturnData->SpecPreload == SpecPreloadOn)
    {

        CP_WSR_Stream->PreloadMode = SstPreloadSpeculative;
        CP_verbose(Stream, "Setting SpeculativePreload ON for new reader\n");
    }

    int MySuccess = initWSReader(CP_WSR_Stream, ReturnData->ReaderCohortSize,
                                 ReturnData->CP_ReaderInfo);

    int GlobalSuccess = 0;
    SMPI_Allreduce(&MySuccess, &GlobalSuccess, 1, MPI_INT, MPI_LAND,
                   Stream->mpiComm);

    if (!GlobalSuccess)
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

    memset(&cpInfo, 0, sizeof(cpInfo));
    cpInfo.ContactInfo = CP_GetContactString(Stream, NULL);
    cpInfo.WriterID = CP_WSR_Stream;

    combined_init.CP_Info = (void **)&cpInfo;
    combined_init.DP_Info = DP_WriterInfo;

    MyStartingTimestep =
        earliestAvailableTimestepNumber(Stream, Stream->WriterTimestep);
    if (MyStartingTimestep == -1)
        MyStartingTimestep = 0;

    SMPI_Allreduce(&MyStartingTimestep, &GlobalStartingTimestep, 1, MPI_LONG,
                   MPI_MAX, Stream->mpiComm);

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
        if (CMwrite(conn, Stream->CPInfo->WriterResponseFormat, &response) != 1)
        {
            CP_verbose(Stream,
                       "Message failed to send to reader in participate in "
                       "reader open\n");
        }
        free(response.CP_WriterInfo);
        free(response.DP_WriterInfo);
    }
    free(cpInfo.ContactInfo);
    if (ret_data_block)
        free(ret_data_block);
    if (pointers)
        free(pointers);
    Stream->NewReaderPresent = 1;
    CP_verbose(Stream,
               "Finish writer-side reader open protocol for reader %p, "
               "reader ready response pending\n",
               CP_WSR_Stream);
    return CP_WSR_Stream;
}

void sendOneToWSRCohort(WS_ReaderInfo CP_WSR_Stream, CMFormat f, void *Msg,
                        void **RS_StreamPtr)
{
    SstStream s = CP_WSR_Stream->ParentStream;
    int j = 0;

    if (s->ConfigParams->CPCommPattern == SstCPCommPeer)
    {
        while (CP_WSR_Stream->Peers[j] != -1)
        {
            int peer = CP_WSR_Stream->Peers[j];
            CMConnection conn = CP_WSR_Stream->Connections[peer].CMconn;
            /* add the reader-rank-specific Stream identifier to each outgoing
             * message */
            *RS_StreamPtr = CP_WSR_Stream->Connections[peer].RemoteStreamID;
            CP_verbose(s, "Sending a message to reader %d (%p)\n", peer,
                       *RS_StreamPtr);
            if ((!conn) || (CMwrite(conn, f, Msg) != 1))
            {
                CP_verbose(s, "Message failed to send to reader %d (%p)\n",
                           peer, *RS_StreamPtr);
            }
            j++;
        }
    }
    else
    {
        /* CommMin */
        if (s->Rank == 0)
        {
            int peer = 0;
            CMConnection conn = CP_WSR_Stream->Connections[peer].CMconn;
            /* add the reader-rank-specific Stream identifier to each outgoing
             * message */
            *RS_StreamPtr = CP_WSR_Stream->Connections[peer].RemoteStreamID;
            CP_verbose(s, "Sending a message to reader %d (%p)\n", peer,
                       *RS_StreamPtr);
            if ((!conn) || (CMwrite(conn, f, Msg) != 1))
            {
                CP_verbose(s, "Message failed to send to reader %d (%p)\n",
                           peer, *RS_StreamPtr);
            }
        }
    }
}

static void AddTSToSentList(SstStream Stream, WS_ReaderInfo Reader,
                            long Timestep)
{
    struct _SentTimestepRec *Item = malloc(sizeof(*Item)),
                            *List = Reader->SentTimestepList;
    Item->Timestep = Timestep;
    Item->Next = NULL;
    if (List == NULL)
    {
        Reader->SentTimestepList = Item;
    }
    else
    {
        while (List->Next != NULL)
        {
            List = List->Next;
        }
        List->Next = Item;
    }
}

static void DerefSentTimestep(SstStream Stream, WS_ReaderInfo Reader,
                              long Timestep)
{
    struct _SentTimestepRec *List = Reader->SentTimestepList, *Last = NULL;
    CP_verbose(Stream, "Reader sent timestep list %p, trying to release %ld\n",
               Reader->SentTimestepList, Timestep);

    while (List)
    {

        int Freed = 0;
        struct _SentTimestepRec *Next = List->Next;
        CP_verbose(
            Stream,
            "Reader considering sent timestep %ld,trying to release %ld\n",
            List->Timestep, Timestep);
        if (List->Timestep == Timestep)
        {
            struct _SentTimestepRec *ItemToFree = List;
            Freed = 1;
            /* per reader release here */
            if (Stream->DP_Interface->readerReleaseTimestep)
            {
                (Stream->DP_Interface->readerReleaseTimestep)(
                    &Svcs, Reader->DP_WSR_Stream, List->Timestep);
            }

            SubRefTimestep(Stream, ItemToFree->Timestep, 1);
            free(ItemToFree);
            if (Last)
            {
                Last->Next = Next;
            }
            else
            {
                Reader->SentTimestepList = Next;
            }
        }
        if (!Freed)
        {
            Last = List;
        }
        List = Next;
    }
}

static void DerefAllSentTimesteps(SstStream Stream, WS_ReaderInfo Reader)
{
    CPTimestepList List = Stream->QueuedTimesteps;

    CP_verbose(Stream, "Dereferencing all timesteps sent to reader %p\n",
               Reader);
    while (List)
    {
        CPTimestepList Next = List->Next;
        CP_verbose(Stream, "Checking on timestep %d\n", List->Timestep);
        DerefSentTimestep(Stream, Reader, List->Timestep);
        List = Next;
    }
    CP_verbose(Stream, "DONE DEREFERENCING\n");
}

static void SendTimestepEntryToSingleReader(SstStream Stream,
                                            CPTimestepList Entry,
                                            WS_ReaderInfo CP_WSR_Stream,
                                            int rank)
{
    SST_ASSERT_LOCKED();
    if (CP_WSR_Stream->ReaderStatus == Established)
    {
        CP_WSR_Stream->LastSentTimestep = Entry->Timestep;
        if (rank != -1)
        {
            CP_verbose(Stream, "Sent timestep %ld to reader cohort %d\n",
                       Entry->Timestep, rank);
        }
        Entry->ReferenceCount++;

        CP_verbose(Stream,
                   "ADDING timestep %ld to sent list for reader cohort %d, "
                   "READER %p, reference count is now %d\n",
                   Entry->Timestep, rank, CP_WSR_Stream, Entry->ReferenceCount);
        AddTSToSentList(Stream, CP_WSR_Stream, Entry->Timestep);
        if (Stream->DP_Interface->readerRegisterTimestep)
        {
            (Stream->DP_Interface->readerRegisterTimestep)(
                &Svcs, CP_WSR_Stream->DP_WSR_Stream, Entry->Timestep,
                CP_WSR_Stream->PreloadMode);
        }

        Entry->Msg->PreloadMode = CP_WSR_Stream->PreloadMode;
        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
        sendOneToWSRCohort(CP_WSR_Stream,
                           Stream->CPInfo->DeliverTimestepMetadataFormat,
                           Entry->Msg, &Entry->Msg->RS_Stream);
        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    }
}

static void SendTimestepEntryToReaders(SstStream Stream, CPTimestepList Entry)
{
    SST_ASSERT_LOCKED();
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        WS_ReaderInfo CP_WSR_Stream = Stream->Readers[i];
        SendTimestepEntryToSingleReader(Stream, Entry, CP_WSR_Stream, i);
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

    /* LOCK */
    /* LastReleased is set to OldestItemTS - 1 */
    /* foreach item in queue */
    /*       if (reader is established) */
    /*           if TS is expired CONTINUE */
    /*          increment TS reference count */
    /* 	 update readers LastSent */
    /*          UNLOCK */
    /*          write event to reader (might cause connection closed) */
    /*          LOCK */
    /*       } */
    /*    } */
    /* } */
    /* UNLOCK */

    /* send any queued metadata necessary */
    Reader->OldestUnreleasedTimestep = Reader->StartingTimestep;
    CP_verbose(Stream,
               "Reader ready on WSR %p, Stream established, Starting %d "
               "LastProvided %d.\n",
               Reader, Reader->StartingTimestep, Stream->LastProvidedTimestep);
    for (long TS = Reader->StartingTimestep; TS <= Stream->LastProvidedTimestep;
         TS++)
    {
        CPTimestepList List = Stream->QueuedTimesteps;
        while (List)
        {
            if (Reader->ReaderStatus != Established)
                continue; /* do nothing if we've fallen out of established */
            if (List->Timestep == TS)
            {
                FFSFormatList SavedFormats = List->Msg->Formats;
                if (List->Expired && !List->PreciousTimestep)
                {
                    CP_verbose(Stream,
                               "Reader send queued skipping  TS %d, expired "
                               "and not precious\n",
                               List->Timestep, TS);
                    List = List->Next;
                    continue; /* do nothing timestep is expired, but not
                                 precious */
                }
                if (TS == Reader->StartingTimestep)
                {
                    /* For first Msg, send all previous formats */
                    List->Msg->Formats = Stream->PreviousFormats;
                }
                CP_verbose(Stream,
                           "Sending Queued TimestepMetadata for timestep %d, "
                           "reference count = %d\n",
                           TS, List->ReferenceCount);

                SendTimestepEntryToSingleReader(Stream, List, Reader, -1);
                if (TS == Reader->StartingTimestep)
                {
                    /* restore Msg format list */
                    List->Msg->Formats = SavedFormats;
                }
            }
            List = List->Next;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

SstStream SstWriterOpen(const char *Name, SstParams Params, MPI_Comm comm)
{
    SstStream Stream;

    Stream = CP_newStream();
    Stream->Role = WriterRole;
    CP_validateParams(Stream, Params, 1 /* Writer */);
    Stream->ConfigParams = Params;

    char *Filename = strdup(Name);

    Stream->mpiComm = comm;

    SMPI_Comm_rank(Stream->mpiComm, &Stream->Rank);
    SMPI_Comm_size(Stream->mpiComm, &Stream->CohortSize);

    Stream->DP_Interface = SelectDP(&Svcs, Stream, Stream->ConfigParams);

    if (!Stream->DP_Interface)
    {
        CP_verbose(Stream, "Failed to load DataPlane %s for Stream \"%s\"\n",
                   Params->DataTransport, Filename);
        return NULL;
    }

    Stream->CPInfo =
        CP_getCPInfo(Stream->DP_Interface, Stream->ConfigParams->ControlModule);

    if (Stream->RendezvousReaderCount > 0)
    {
        Stream->FirstReaderCondition =
            CMCondition_get(Stream->CPInfo->cm, NULL);
    }
    else
    {
        Stream->FirstReaderCondition = -1;
    }

    attr_list DPAttrs = create_attr_list();
    Stream->DP_Stream = Stream->DP_Interface->initWriter(
        &Svcs, Stream, Stream->ConfigParams, DPAttrs);

    if (Stream->Rank == 0)
    {
        registerContactInfo(Filename, Stream, DPAttrs);
    }

    CP_verbose(Stream, "Opening Stream \"%s\"\n", Filename);

    if (Stream->Rank == 0)
    {
        CP_verbose(Stream, "Writer stream params are:\n");
        CP_dumpParams(Stream, Stream->ConfigParams, 0 /* writer side */);
    }

    if (globalNetinfoCallback)
    {
        (globalNetinfoCallback)(0, CP_GetContactString(Stream, DPAttrs),
                                IPDiagString);
    }
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
        SMPI_Barrier(Stream->mpiComm);

        struct timeval Start, Stop, Diff;
        gettimeofday(&Start, NULL);
        reader = WriterParticipateInReaderOpen(Stream);
        if (!reader)
        {
            CP_error(Stream, "Potential reader registration failed\n");
            break;
        }
        if (Stream->ConfigParams->CPCommPattern == SstCPCommPeer)
        {
            waitForReaderResponseAndSendQueued(reader);
            SMPI_Barrier(Stream->mpiComm);
        }
        else
        {
            if (Stream->Rank == 0)
            {
                waitForReaderResponseAndSendQueued(reader);
                SMPI_Bcast(&reader->ReaderStatus, 1, MPI_INT, 0,
                           Stream->mpiComm);
            }
            else
            {
                SMPI_Bcast(&reader->ReaderStatus, 1, MPI_INT, 0,
                           Stream->mpiComm);
            }
        }
        Stream->RendezvousReaderCount--;
    }
    Stream->Filename = Filename;
    Stream->Status = Established;
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
    SstStream ParentStream = CP_WSR_Stream->ParentStream;

    PTHREAD_MUTEX_LOCK(&ParentStream->DataLock);
    CP_PeerFailCloseWSReader(CP_WSR_Stream, PeerClosed);
    PTHREAD_MUTEX_UNLOCK(&ParentStream->DataLock);
}

static void CP_PeerFailCloseWSReader(WS_ReaderInfo CP_WSR_Stream,
                                     enum StreamStatus NewState)
{
    SstStream ParentStream = CP_WSR_Stream->ParentStream;
    SST_ASSERT_LOCKED();
    if (ParentStream->Status != Established)
    {
        CP_verbose(
            ParentStream,
            "In PeerFailCloseWSReader, but Parent status not Established, %d\n",
            ParentStream->Status);
        return;
    }

    if (CP_WSR_Stream->ReaderStatus == NewState)
    {
        CP_verbose(ParentStream,
                   "In PeerFailCloseWSReader, but status is already set% d\n",
                   ParentStream->Status);
        return;
    }

    if ((NewState == PeerClosed) || (NewState == Closed))
    {
        CP_verbose(ParentStream,
                   "In PeerFailCloseWSReader, releasing sent timesteps\n");
        DerefAllSentTimesteps(CP_WSR_Stream->ParentStream, CP_WSR_Stream);
        CP_WSR_Stream->OldestUnreleasedTimestep =
            CP_WSR_Stream->LastSentTimestep + 1;
        for (int i = 0; i < CP_WSR_Stream->ReaderCohortSize; i++)
        {
            if (CP_WSR_Stream->Connections[i].CMconn)
            {
                CMConnection_close(CP_WSR_Stream->Connections[i].CMconn);
                CP_WSR_Stream->Connections[i].CMconn = NULL;
            }
        }
    }
    if (NewState == PeerFailed)
    {
        DerefAllSentTimesteps(CP_WSR_Stream->ParentStream, CP_WSR_Stream);
        CMadd_delayed_task(ParentStream->CPInfo->cm, 2, 0, CloseWSRStream,
                           CP_WSR_Stream);
    }
    CP_verbose(ParentStream, "Moving Reader stream %p to status %s\n",
               CP_WSR_Stream, SSTStreamStatusStr[NewState]);
    CP_WSR_Stream->ReaderStatus = NewState;
    pthread_cond_signal(&ParentStream->DataCondition);

    QueueMaintenance(ParentStream);
}

/*
On writer close:
   RemovePreciousTag on any timestep in queue
   Set Reserve count to 0
   on rank 0:
      LOCK
      queue maintenance
      while (timestep queue not empty)
        WAIT
      }
      UNLOCK
   Barrier()
*/
void SstWriterClose(SstStream Stream)
{
    struct _WriterCloseMsg Msg;
    struct timeval CloseTime, Diff;
    Msg.FinalTimestep = Stream->LastProvidedTimestep;
    sendOneToEachReaderRank(Stream, Stream->CPInfo->WriterCloseFormat, &Msg,
                            &Msg.RS_Stream);

    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    UntagPreciousTimesteps(Stream);
    Stream->ConfigParams->ReserveQueueLimit = 0;
    QueueMaintenance(Stream);

    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);

    // sleep briefly to allow for outgoing close messages to arrive
    usleep(100 * 1000);

    if ((Stream->ConfigParams->CPCommPattern == SstCPCommPeer) ||
        (Stream->Rank == 0))
    {
        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
        while (Stream->QueuedTimesteps)
        {
            CP_verbose(Stream,
                       "Waiting for timesteps to be released in WriterClose\n");
            if (Stream->CPVerbose)
            {
                CPTimestepList List = Stream->QueuedTimesteps;
                char *StringList = malloc(1);
                StringList[0] = 0;

                while (List)
                {
                    char tmp[20];
                    CP_verbose(Stream,
                               "IN TS WAIT, ENTRIES areTimestep %ld (exp %d, "
                               "Prec %d, Ref %d), Count now %d\n",
                               List->Timestep, List->Expired,
                               List->PreciousTimestep, List->ReferenceCount,
                               Stream->QueuedTimestepCount);
                    sprintf(tmp, "%ld ", List->Timestep);
                    StringList = realloc(StringList,
                                         strlen(StringList) + strlen(tmp) + 1);
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
                CP_verbose(
                    Stream, "Reader [%d] status is %s\n", i,
                    SSTStreamStatusStr[Stream->Readers[i]->ReaderStatus]);
            }
            /* NEED TO HANDLE FAILURE HERE */
            pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
        }
        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    }
    if (Stream->ConfigParams->CPCommPattern == SstCPCommMin)
    {
        /*
         * if we're CommMin, getting here implies that Rank 0 has released all
         * timesteps, other ranks can follow suit after barrier
         */
        SMPI_Barrier(Stream->mpiComm);
        ReleaseAndDiscardRemainingTimesteps(Stream);
    }
    gettimeofday(&CloseTime, NULL);
    timersub(&CloseTime, &Stream->ValidStartTime, &Diff);
    if (Stream->Stats)
        Stream->Stats->ValidTimeSecs = (double)Diff.tv_usec / 1e6 + Diff.tv_sec;

    CP_verbose(Stream, "All timesteps are released in WriterClose\n");

    /*
     *  Only rank 0 removes contact info, and only when everything is closed.
     */
    if (Stream->Rank == 0)
    {
        removeContactInfo(Stream);
    }
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

static void *FillMetadataMsg(SstStream Stream, struct _TimestepMetadataMsg *Msg,
                             MetadataPlusDPInfo *pointers)
{
    FFSFormatList XmitFormats = NULL;
    void *MetadataFreeValue = NULL;

    /* build the Metadata Msg */
    Msg->CohortSize = Stream->CohortSize;
    Msg->Timestep = Stream->WriterTimestep;

    /* separate metadata and DP_info to separate arrays */
    Msg->Metadata = malloc(Stream->CohortSize * sizeof(Msg->Metadata[0]));
    Msg->AttributeData = malloc(Stream->CohortSize * sizeof(Msg->Metadata[0]));
    Msg->DP_TimestepInfo =
        malloc(Stream->CohortSize * sizeof(Msg->DP_TimestepInfo[0]));
    int NullCount = 0;
    for (int i = 0; i < Stream->CohortSize; i++)
    {
        if (pointers[i]->Metadata)
        {
            Msg->Metadata[i] = *(pointers[i]->Metadata);
        }
        else
        {
            Msg->Metadata[i].DataSize = 0;
            Msg->Metadata[i].block = NULL;
        }
        if (pointers[i]->AttributeData)
        {
            Msg->AttributeData[i] = *(pointers[i]->AttributeData);
        }
        else
        {
            Msg->AttributeData[i].DataSize = 0;
            Msg->AttributeData[i].block = NULL;
        }
        Msg->DP_TimestepInfo[i] = pointers[i]->DP_TimestepInfo;
        if (pointers[i]->DP_TimestepInfo == NULL)
            NullCount++;
        XmitFormats = AddUniqueFormats(XmitFormats, pointers[i]->Formats,
                                       /*nocopy*/ 0);
    }
    if (NullCount == Stream->CohortSize)
    {
        free(Msg->DP_TimestepInfo);
        Msg->DP_TimestepInfo = NULL;
    }

    if (Stream->AssembleMetadataUpcall)
    {
        MetadataFreeValue = Stream->AssembleMetadataUpcall(
            Stream->UpcallWriter, Stream->CohortSize, Msg->Metadata,
            Msg->AttributeData);
        /* Assume rank 0 values alone are useful now, zero others */
        for (int i = 1; i < Stream->CohortSize; i++)
        {
            Msg->Metadata[i].DataSize = 0;
            Msg->Metadata[i].block = NULL;
            Msg->AttributeData[i].DataSize = 0;
            Msg->AttributeData[i].block = NULL;
        }
    }

    free(pointers);

    Stream->PreviousFormats =
        AddUniqueFormats(Stream->PreviousFormats, XmitFormats, /*copy*/ 1);

    if (Stream->NewReaderPresent)
    {
        /*
         *  If there is a new reader cohort, those ranks will need all prior
         * FFS
         * Format info.
         */
        Msg->Formats = Stream->PreviousFormats;
        Stream->NewReaderPresent = 0;
    }
    else
    {
        Msg->Formats = XmitFormats;
    }
    return MetadataFreeValue;
}

static void ProcessReaderStatusList(SstStream Stream,
                                    ReturnMetadataInfo Metadata)
{
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    for (int i = 0; i < Metadata->ReaderCount; i++)
    {
        if (Stream->Readers[i]->ReaderStatus != Metadata->ReaderStatus[i])
        {
            CP_verbose(Stream, "Adjusting reader %d status from %s to %s\n", i,
                       SSTStreamStatusStr[Stream->Readers[i]->ReaderStatus],
                       SSTStreamStatusStr[Metadata->ReaderStatus[i]]);
            CP_PeerFailCloseWSReader(Stream->Readers[i],
                                     Metadata->ReaderStatus[i]);
        }
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

static void ActOnTSLockStatus(SstStream Stream)
{
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        if ((Stream->WriterDefinitionsLocked) &&
            (Stream->Readers[i]->ReaderDefinitionsLocked == 1))
        {
            struct _CommPatternLockedMsg Msg;
            if (Stream->DP_Interface->WSRreadPatternLocked)
            {
                Stream->DP_Interface->WSRreadPatternLocked(
                    &Svcs, Stream->Readers[i]->DP_WSR_Stream,
                    Stream->Readers[i]->ReaderSelectionLockTimestep);
            }
            Msg.Timestep = Stream->Readers[i]->ReaderSelectionLockTimestep;
            sendOneToWSRCohort(Stream->Readers[i],
                               Stream->CPInfo->CommPatternLockedFormat, &Msg,
                               &Msg.RS_Stream);
            Stream->Readers[i]->ReaderDefinitionsLocked = 2;
            Stream->Readers[i]->PreloadMode = SstPreloadLearned;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

static void ProcessReleaseList(SstStream Stream, ReturnMetadataInfo Metadata)
{
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    for (int i = 0; i < Metadata->ReleaseCount; i++)
    {
        CPTimestepList List = Stream->QueuedTimesteps;
        CP_verbose(Stream, "Release List, TS %ld\n",
                   Metadata->ReleaseList[i].Timestep);
        while (List)
        {
            if (List->Timestep == Metadata->ReleaseList[i].Timestep)
            {
                /* find local reader that matches global reader and notify local
                 * DP of release */
                int j;
                for (j = 0; j < Stream->ReaderCount; j++)
                {
                    if (Stream->Readers[j]->RankZeroID ==
                        Metadata->ReleaseList[i].Reader)
                    {
                        break;
                    }
                }
                assert(j < Stream->ReaderCount);
                if (List->Timestep > Stream->Readers[j]->LastReleasedTimestep)
                {
                    CP_verbose(Stream,
                               "Updating reader %d last released to %ld\n", j,
                               List->Timestep);
                    Stream->Readers[j]->LastReleasedTimestep = List->Timestep;
                }
                CP_verbose(Stream,
                           "Release List, and set ref count of timestep %ld\n",
                           Metadata->ReleaseList[i].Timestep);
                /* per reader release here */
                if (Stream->DP_Interface->readerReleaseTimestep)
                {
                    (Stream->DP_Interface->readerReleaseTimestep)(
                        &Svcs, Stream->Readers[j]->DP_WSR_Stream,
                        List->Timestep);
                }

                List->ReferenceCount = 0;
            }
            List = List->Next;
        }
    }
    QueueMaintenance(Stream);
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

static void ProcessLockDefnsList(SstStream Stream, ReturnMetadataInfo Metadata)
{
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    for (int i = 0; i < Metadata->LockDefnsCount; i++)
    {
        CPTimestepList List = Stream->QueuedTimesteps;
        CP_verbose(Stream, "LockDefns List, TS %ld\n",
                   Metadata->LockDefnsList[i].Timestep);
        while (List)
        {
            if (List->Timestep == Metadata->LockDefnsList[i].Timestep)
            {
                /* find local reader that matches global reader and notify local
                 * DP of definition lock */
                int j;
                for (j = 0; j < Stream->ReaderCount; j++)
                {
                    if (Stream->Readers[j]->RankZeroID ==
                        Metadata->LockDefnsList[i].Reader)
                    {
                        break;
                    }
                }
                assert(j < Stream->ReaderCount);
                WS_ReaderInfo Reader = (WS_ReaderInfo)Stream->Readers[j];

                Reader->ReaderDefinitionsLocked = 1;
                CP_verbose(Stream, "LockDefns List, FOUND TS %ld\n",
                           Metadata->LockDefnsList[i].Timestep);
            }
            List = List->Next;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
}

/*
 *
Protocol notes:



Single unified "queue" of timesteps.  Discard only occurs at head or at tail.
The readers all maintain an "active timestep" in the queue.  The set of
timesteps older than any reader's active pointer is the "reserve queue", the
default reserve queue size is 0.

Entities protected with LOCK
         Timestep queue and all properties of items in it
         Reader list and their status


Upon TimestepProvision:
        LOCK
        Register data with the data plane
        Make an entry in the queue with reference count 0.
        UNLOCK

        Aggregate to rank 0:
                all meta data

        (RANK 0 only)
        if (blockingMode) {
                LOCK
                while (overall queue length > limit) {
                        WAIT (implict unlock)
                }
                UNLOCK
        }


        Distribute from rank 0:
                this timestep discard decision.
                if (not discard && not CommMin) aggregated metadata
                release list
                Waiting reader count

        LOCK
        if discard {
           dequeue just-added timestep
           unregister data with the data plane
           free data
        } else {
           foreach reader
              if (reader is established)
                 increment TS reference count
                 update readers LastSent
                 UNLOCK
                 write event to reader (might cause connection closed)
                 LOCK
              }
           }
        }
        if (rank != 0)
           handle release timestep list
        }
        UNLOCK
        Handle new readers

Queue maintenance:    (ASSUME LOCKED)
        calculate largest entry for CurrentTimestep in a reader.  Update that as
OldestCurrentReaderTimestep. If any timestep has zero ref count and is
registered with DP deregister that timestep with DP CallRemoveQueueEntries

RemoveQueueEntries:
        If the number of timesteps older than OldestCurrentReaderTimestep, mark
them as Expired Dequeue and free any timestep that is Expired, not Precious and
has reference count 0. if change SIGNAL


On writer close:
   RemovePreciousTag on any timestep in queue
   Set Reserve count to 0
   on rank 0:
      LOCK
      queue maintenance
      while (timestep queue not empty)
        WAIT
      }
      UNLOCK
   Barrier()

Asynchronous actions:

Arrival of ReleaseTimestep message:
        LOCK
        Decremement reference count on queueitem:
        update Reader LastReleased item
        notify DP of per-reader release
        if (CommMin)
           // must be rank 0
           add reader/TS pair to release-list to notify other writer ranks.
        QueueMaintenance
        UNLOCK

Receipt of a "ConnectionClosed" event
        LOCK
        decrement reference count on timesteps between LastReleased and LastSent
        LastSent = -1; LastReleased = -1;
        QueueMaintenance
        UNLOCK


On new reader:
        LOCK
        LastReleased is set to OldestItemTS - 1
        foreach item in queue
              if (reader is established)
                 increment TS reference count
                 update readers LastSent
                 UNLOCK
                 write event to reader (might cause connection closed)
                 LOCK
              }
           }
        }
        UNLOCK

on reader close:
   LOCK
   update reader status to closed
   UNLOCK


 */
extern void SstInternalProvideTimestep(
    SstStream Stream, SstData LocalMetadata, SstData Data, long Timestep,
    FFSFormatList Formats, DataFreeFunc FreeTimestep, void *FreeClientData,
    SstData AttributeData, DataFreeFunc FreeAttributeData,
    void *FreeAttributelientData)
{
    void *data_block1, *data_block2;
    MetadataPlusDPInfo *pointers;
    ReturnMetadataInfo ReturnData;
    struct _ReturnMetadataInfo TimestepMetaData;
    struct _TimestepMetadataMsg *Msg = malloc(sizeof(*Msg));
    void *DP_TimestepInfo = NULL;
    struct _MetadataPlusDPInfo Md;
    CPTimestepList Entry = calloc(1, sizeof(struct _CPTimestepEntry));
    int GlobalOpRequested = 0;
    int GlobalOpRequired = 0;
    int PendingReaderCount = 0;

    memset(Msg, 0, sizeof(*Msg));
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    Stream->WriterTimestep = Timestep;

    Stream->DP_Interface->provideTimestep(&Svcs, Stream->DP_Stream, Data,
                                          LocalMetadata, Timestep,
                                          &DP_TimestepInfo);

    /* Md is the local contribution to MetaData */
    Md.Formats = Formats;
    Md.Metadata = (SstData)LocalMetadata;
    Md.AttributeData = (SstData)AttributeData;
    Md.DP_TimestepInfo = DP_TimestepInfo;

    if (Data)
        TAU_SAMPLE_COUNTER("Timestep local data size", Data->DataSize);
    if (LocalMetadata)
        TAU_SAMPLE_COUNTER("Timestep local metadata size",
                           LocalMetadata->DataSize);

    /* preliminary queue of message before metadata collection.  Timestep may
     * still be discarded.*/

    Stream->LastProvidedTimestep = Timestep;
    if (Stream->ConfigParams->FirstTimestepPrecious && (Timestep == 0))
    {
        Entry->PreciousTimestep = 1;
    }
    Entry->ReferenceCount =
        1; /* holding one for us, so it doesn't disappear under us */
    Entry->DPRegistered = 1;
    Entry->Timestep = Timestep;
    Entry->Msg = Msg;
    Entry->MetadataArray = Msg->Metadata;
    Entry->DP_TimestepInfo = Msg->DP_TimestepInfo;
    Entry->FreeTimestep = FreeTimestep;
    Entry->FreeClientData = FreeClientData;
    Entry->Next = Stream->QueuedTimesteps;
    Stream->QueuedTimesteps = Entry;
    Stream->QueuedTimestepCount++;
    /* no one waits on timesteps being added, so no condition signal to note
     * change */

    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);

    TAU_START("Metadata Consolidation time in EndStep()");
    pointers = (MetadataPlusDPInfo *)CP_consolidateDataToRankZero(
        Stream, &Md, Stream->CPInfo->PerRankMetadataFormat, &data_block1);

    if (Stream->Rank == 0)
    {
        int DiscardThisTimestep = 0;
        struct _ReturnMetadataInfo TimestepMetaData;
        RequestQueue ArrivingReader = Stream->ReadRequestQueue;
        void *MetadataFreeValue;
        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
        QueueMaintenance(Stream);
        if (Stream->QueueFullPolicy == SstQueueFullDiscard)
        {
            CP_verbose(Stream,
                       "Testing Discard Condition, Queued Timestep Count %d, "
                       "QueueLimit %d\n",
                       Stream->QueuedTimestepCount, Stream->QueueLimit);
            QueueMaintenance(Stream);
            if (Stream->QueuedTimestepCount > Stream->QueueLimit)
            {
                DiscardThisTimestep = 1;
            }
        }
        else
        {
            while ((Stream->QueueLimit > 0) &&
                   (Stream->QueuedTimestepCount > Stream->QueueLimit))
            {
                CP_verbose(Stream, "Blocking on QueueFull condition\n");
                pthread_cond_wait(&Stream->DataCondition, &Stream->DataLock);
            }
        }
        TimestepMetaData.PendingReaderCount = 0;
        while (ArrivingReader)
        {
            TimestepMetaData.PendingReaderCount++;
            ArrivingReader = ArrivingReader->Next;
        }

        TimestepMetaData.DiscardThisTimestep = DiscardThisTimestep;
        TimestepMetaData.ReleaseCount = Stream->ReleaseCount;
        TimestepMetaData.ReleaseList = Stream->ReleaseList;
        TimestepMetaData.LockDefnsCount = Stream->LockDefnsCount;
        TimestepMetaData.LockDefnsList = Stream->LockDefnsList;
        TimestepMetaData.ReaderStatus =
            malloc(sizeof(enum StreamStatus) * Stream->ReaderCount);
        TimestepMetaData.ReaderCount = Stream->ReaderCount;
        for (int i = 0; i < Stream->ReaderCount; i++)
        {
            TimestepMetaData.ReaderStatus[i] = Stream->Readers[i]->ReaderStatus;
        }
        Stream->ReleaseCount = 0;
        Stream->ReleaseList = NULL;
        Stream->LockDefnsCount = 0;
        Stream->LockDefnsList = NULL;
        MetadataFreeValue =
            FillMetadataMsg(Stream, &TimestepMetaData.Msg, pointers);
        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
        ReturnData = CP_distributeDataFromRankZero(
            Stream, &TimestepMetaData, Stream->CPInfo->ReturnMetadataInfoFormat,
            &data_block2);
        if (Stream->FreeMetadataUpcall)
        {
            Stream->FreeMetadataUpcall(Stream->UpcallWriter, Msg->Metadata,
                                       Msg->AttributeData, MetadataFreeValue);
        }
        free(TimestepMetaData.ReleaseList);
        free(TimestepMetaData.ReaderStatus);
        free(TimestepMetaData.LockDefnsList);
        free(TimestepMetaData.Msg.Metadata);
        free(TimestepMetaData.Msg.AttributeData);
    }
    else
    {
        /* other ranks */
        ReturnData = CP_distributeDataFromRankZero(
            Stream, NULL, Stream->CPInfo->ReturnMetadataInfoFormat,
            &data_block2);
        Stream->PreviousFormats = AddUniqueFormats(
            Stream->PreviousFormats, ReturnData->Msg.Formats, /*copy*/ 1);
    }
    free(data_block1);
    PendingReaderCount = ReturnData->PendingReaderCount;
    *Msg = ReturnData->Msg;
    Msg->CohortSize = Stream->CohortSize;
    Msg->Timestep = Timestep;
    TAU_STOP("Metadata Consolidation time in EndStep()");

    /*
     * lock this Stream's data and queue the timestep
     */
    Entry->Msg = Msg;
    Entry->MetadataArray = Msg->Metadata;
    Entry->DP_TimestepInfo = Msg->DP_TimestepInfo;
    Entry->DataBlockToFree = data_block2;

    ProcessReaderStatusList(Stream, ReturnData);

    if ((Stream->ConfigParams->CPCommPattern == SstCPCommMin) &&
        (Stream->Rank != 0))
    {
        ProcessLockDefnsList(Stream, ReturnData);
        ProcessReleaseList(Stream, ReturnData);
    }
    ActOnTSLockStatus(Stream);
    TAU_START("provide timestep operations");
    if (ReturnData->DiscardThisTimestep)
    {
        /* Data was actually discarded, but we want to send a message to each
         * reader so that it knows a step was discarded, but actually so that we
         * get an error return if the write fails */

        Msg->Metadata = NULL;
        Msg->DP_TimestepInfo = NULL;

        CP_verbose(Stream,
                   "Sending Empty TimestepMetadata for Discarded "
                   "timestep %d, one to each reader\n",
                   Timestep);

        sendOneToEachReaderRank(Stream,
                                Stream->CPInfo->DeliverTimestepMetadataFormat,
                                Msg, &Msg->RS_Stream);

        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
        Entry->Expired = 1;
        Entry->ReferenceCount = 0;
        QueueMaintenance(Stream);
        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    }
    else
    {

        CP_verbose(Stream,
                   "Sending TimestepMetadata for timestep %d (ref count "
                   "%d), one to each reader\n",
                   Timestep, Entry->ReferenceCount);

        PTHREAD_MUTEX_LOCK(&Stream->DataLock);
        SendTimestepEntryToReaders(Stream, Entry);
        SubRefTimestep(Stream, Entry->Timestep, 0);
        QueueMaintenance(Stream);
        PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    }
    while (PendingReaderCount--)
    {
        WS_ReaderInfo reader;
        CP_verbose(Stream,
                   "Writer side ReaderLateArrival accepting incoming reader\n");
        reader = WriterParticipateInReaderOpen(Stream);
        if (!reader)
        {
            CP_error(Stream, "Potential reader registration failed\n");
            break;
        }
        if (Stream->ConfigParams->CPCommPattern == SstCPCommPeer)
        {
            waitForReaderResponseAndSendQueued(reader);
        }
        else
        {
            if (Stream->Rank == 0)
            {
                waitForReaderResponseAndSendQueued(reader);
                SMPI_Bcast(&reader->ReaderStatus, 1, MPI_INT, 0,
                           Stream->mpiComm);
            }
            else
            {
                SMPI_Bcast(&reader->ReaderStatus, 1, MPI_INT, 0,
                           Stream->mpiComm);
            }
        }
    }
    TAU_STOP("provide timestep operations");
}

extern void SstWriterDefinitionLock(SstStream Stream, long EffectiveTimestep)
{
    PTHREAD_MUTEX_LOCK(&Stream->DataLock);
    Stream->WriterDefinitionsLocked = 1;
    for (int i = 0; i < Stream->ReaderCount; i++)
    {
        if (Stream->Readers[i]->ReaderDefinitionsLocked == 1)
        {
            if ((Stream->Rank == 0) &&
                (Stream->ConfigParams->CPCommPattern == SstCPCommMin))
            {
                Stream->LockDefnsList = realloc(
                    Stream->LockDefnsList, sizeof(Stream->LockDefnsList[0]) *
                                               (Stream->LockDefnsCount + 1));
                Stream->LockDefnsList[Stream->LockDefnsCount].Timestep =
                    EffectiveTimestep;
                Stream->LockDefnsList[Stream->LockDefnsCount].Reader =
                    Stream->Readers[i];
                Stream->LockDefnsCount++;
            }
            if (Stream->ConfigParams->CPCommPattern == SstCPCommPeer)
            {
                Stream->Readers[i]->ReaderSelectionLockTimestep =
                    EffectiveTimestep;
            }
        }
    }
    PTHREAD_MUTEX_UNLOCK(&Stream->DataLock);
    CP_verbose(Stream, "Writer-side definitions lock as of timestep %d\n",
               EffectiveTimestep);
}

extern void SstProvideTimestep(SstStream Stream, SstData LocalMetadata,
                               SstData Data, long Timestep,
                               DataFreeFunc FreeTimestep, void *FreeClientData,
                               SstData AttributeData,
                               DataFreeFunc FreeAttributeData,
                               void *FreeAttributeClientData)
{
    SstInternalProvideTimestep(Stream, LocalMetadata, Data, Timestep, NULL,
                               FreeTimestep, FreeClientData, AttributeData,
                               FreeAttributeData, FreeAttributeClientData);
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
    TAU_START_FUNC();
    struct _ReaderCloseMsg *Msg = (struct _ReaderCloseMsg *)Msg_v;

    WS_ReaderInfo CP_WSR_Stream = Msg->WSR_Stream;
    if ((CP_WSR_Stream->ParentStream == NULL) ||
        (CP_WSR_Stream->ParentStream->Status != Established))
        return;

    CP_verbose(CP_WSR_Stream->ParentStream,
               "Reader Close message received for stream %p.  Setting state to "
               "PeerClosed and releasing timesteps.\n",
               CP_WSR_Stream);
    PTHREAD_MUTEX_LOCK(&CP_WSR_Stream->ParentStream->DataLock);
    CP_PeerFailCloseWSReader(CP_WSR_Stream, PeerClosed);
    PTHREAD_MUTEX_UNLOCK(&CP_WSR_Stream->ParentStream->DataLock);
    TAU_STOP_FUNC();
}

void CP_ReaderRegisterHandler(CManager cm, CMConnection conn, void *Msg_v,
                              void *client_data, attr_list attrs)
{
    TAU_REGISTER_THREAD();
    TAU_START_FUNC();
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
    TAU_STOP_FUNC();
}

void CP_ReaderActivateHandler(CManager cm, CMConnection conn, void *Msg_v,
                              void *client_data, attr_list attrs)
{
    TAU_START_FUNC();
    struct _ReaderActivateMsg *Msg = (struct _ReaderActivateMsg *)Msg_v;

    WS_ReaderInfo CP_WSR_Stream = Msg->WSR_Stream;
    CP_verbose(CP_WSR_Stream->ParentStream,
               "Reader Activate message received "
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
    TAU_STOP_FUNC();
}

extern void CP_ReleaseTimestepHandler(CManager cm, CMConnection conn,
                                      void *Msg_v, void *client_data,
                                      attr_list attrs)
{
    TAU_START_FUNC();
    struct _ReleaseTimestepMsg *Msg = (struct _ReleaseTimestepMsg *)Msg_v;
    WS_ReaderInfo Reader = (WS_ReaderInfo)Msg->WSR_Stream;
    SstStream ParentStream = Reader->ParentStream;
    CPTimestepList Entry = NULL;
    int ReaderNum = -1;

    for (int i = 0; i < ParentStream->ReaderCount; i++)
    {
        if (Reader == ParentStream->Readers[i])
        {
            ReaderNum = i;
        }
    }
    CP_verbose(ParentStream,
               "Received a release timestep message "
               "for timestep %d from reader cohort %d\n",
               Msg->Timestep, ReaderNum);

    /* decrement the reference count for the released timestep */
    PTHREAD_MUTEX_LOCK(&ParentStream->DataLock);
    Reader->LastReleasedTimestep = Msg->Timestep;
    if ((ParentStream->Rank == 0) &&
        (ParentStream->ConfigParams->CPCommPattern == SstCPCommMin))
    {
        ParentStream->ReleaseList = realloc(
            ParentStream->ReleaseList, sizeof(ParentStream->ReleaseList[0]) *
                                           (ParentStream->ReleaseCount + 1));
        ParentStream->ReleaseList[ParentStream->ReleaseCount].Timestep =
            Msg->Timestep;
        ParentStream->ReleaseList[ParentStream->ReleaseCount].Reader = Reader;
        ParentStream->ReleaseCount++;
    }
    DerefSentTimestep(ParentStream, Reader, Msg->Timestep);
    QueueMaintenance(ParentStream);
    Reader->OldestUnreleasedTimestep = Msg->Timestep + 1;
    pthread_cond_signal(&ParentStream->DataCondition);
    PTHREAD_MUTEX_UNLOCK(&ParentStream->DataLock);
    TAU_STOP_FUNC();
}

extern void CP_LockReaderDefinitionsHandler(CManager cm, CMConnection conn,
                                            void *Msg_v, void *client_data,
                                            attr_list attrs)
{
    TAU_START_FUNC();
    struct _ReleaseTimestepMsg *Msg = (struct _ReleaseTimestepMsg *)Msg_v;
    WS_ReaderInfo Reader = (WS_ReaderInfo)Msg->WSR_Stream;
    SstStream ParentStream = Reader->ParentStream;
    CPTimestepList Entry = NULL;
    int ReaderNum = -1;

    for (int i = 0; i < ParentStream->ReaderCount; i++)
    {
        if (Reader == ParentStream->Readers[i])
        {
            ReaderNum = i;
        }
    }
    CP_verbose(ParentStream,
               "Received a lock reader definitions message "
               "for timestep %d from reader cohort %d\n",
               Msg->Timestep, ReaderNum);

    PTHREAD_MUTEX_LOCK(&ParentStream->DataLock);
    if ((ParentStream->Rank == 0) &&
        (ParentStream->ConfigParams->CPCommPattern == SstCPCommMin))
    {
        ParentStream->LockDefnsList =
            realloc(ParentStream->LockDefnsList,
                    sizeof(ParentStream->LockDefnsList[0]) *
                        (ParentStream->LockDefnsCount + 1));
        ParentStream->LockDefnsList[ParentStream->LockDefnsCount].Timestep =
            Msg->Timestep;
        ParentStream->LockDefnsList[ParentStream->LockDefnsCount].Reader =
            Reader;
        ParentStream->LockDefnsCount++;
    }
    if ((ParentStream->Rank == 0) ||
        (ParentStream->ConfigParams->CPCommPattern == SstCPCommPeer))
    {
        Reader->ReaderDefinitionsLocked = 1;
        if (ParentStream->WriterDefinitionsLocked)
        {
            Reader->ReaderSelectionLockTimestep = Msg->Timestep;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&ParentStream->DataLock);
    TAU_STOP_FUNC();
}

void SstWriterInitMetadataCallback(SstStream Stream, void *Writer,
                                   AssembleMetadataUpcallFunc AssembleCallback,
                                   FreeMetadataUpcallFunc FreeCallback)
{
    Stream->AssembleMetadataUpcall = AssembleCallback;
    Stream->FreeMetadataUpcall = FreeCallback;
    Stream->UpcallWriter = Writer;
}
