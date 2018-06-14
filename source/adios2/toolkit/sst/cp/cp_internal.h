#include "dp_interface.h"
#include <pthread.h>

typedef struct _CP_GlobalInfo
{
    /* exchange info */
    CManager cm;
    FFSContext ffs_c;
    FMContext fm_c;
    FFSTypeHandle PerRankReaderInfoFormat;
    FFSTypeHandle CombinedReaderInfoFormat;
    CMFormat ReaderRegisterFormat;
    FFSTypeHandle PerRankWriterInfoFormat;
    FFSTypeHandle CombinedWriterInfoFormat;
    CMFormat WriterResponseFormat;
    FFSTypeHandle PerRankMetadataFormat;
    CMFormat DeliverTimestepMetadataFormat;
    CMFormat ReaderActivateFormat;
    CMFormat ReleaseTimestepFormat;
    CMFormat WriterCloseFormat;
    CMFormat ReaderCloseFormat;
    int CustomStructCount;
    FMStructDescList *CustomStructList;
    int LastCallFreeCount;
    void **LastCallFreeList;
} * CP_GlobalInfo;

struct _ReaderRegisterMsg;

typedef struct _RequestQueue
{
    struct _ReaderRegisterMsg *Msg;
    CMConnection Conn;
    struct _RequestQueue *Next;
} * RequestQueue;

typedef struct _CP_PeerConnection
{
    attr_list ContactList;
    void *RemoteStreamID;
    CMConnection CMconn;
} CP_PeerConnection;

enum StreamStatus
{
    NotOpen = 0,
    Established,
    PeerClosed,
    PeerFailed,
    Closed
};

typedef struct _WS_ReaderInfo
{
    SstStream ParentStream;
    enum StreamStatus ReaderStatus;
    long StartingTimestep;
    long LastSentTimestep;
    void *DP_WSR_Stream;
    void *RS_StreamID;
    int ReaderCohortSize;
    int *Peers;
    CP_PeerConnection *Connections;
} * WS_ReaderInfo;

typedef struct _TimestepMetadataList
{
    struct _TimestepMetadataMsg *MetadataMsg;
    struct _TimestepMetadataList *Next;
} * TSMetadataList;

enum StreamRole
{
    ReaderRole,
    WriterRole
};

typedef struct _CPTimestepEntry
{
    long Timestep;
    struct _SstData Data;
    struct _TimestepMetadataMsg *Msg;
    int ReferenceCount;
    void **DP_TimestepInfo;
    SstData *MetadataArray;
    DataFreeFunc FreeTimestep;
    void *FreeClientData;
    void *DataBlockToFree;
    struct _CPTimestepEntry *Next;
} * CPTimestepList;

typedef struct FFSFormatBlock *FFSFormatList;

struct _SstStream
{
    CP_GlobalInfo CPInfo;

    MPI_Comm mpiComm;
    enum StreamRole Role;

    /* params */
    int RendezvousReaderCount;
    char *DataTransport;
    SstRegistrationMethod RegistrationMethod;

    /* state */
    int Verbose;
    double OpenTimeSecs;
    struct timeval ValidStartTime;
    SstStats Stats;

    /* MPI info */
    int Rank;
    int CohortSize;

    CP_DP_Interface DP_Interface;
    void *DP_Stream;

    pthread_mutex_t DataLock;
    pthread_cond_t DataCondition;
    SstParams ConfigParams;

    /* WRITER-SIDE FIELDS */
    int WriterTimestep;
    int ReaderTimestep;
    CPTimestepList QueuedTimesteps;
    int QueuedTimestepCount;
    int QueueLimit;
    int DiscardOnQueueFull;
    int LastProvidedTimestep;
    int NewReaderPresent;

    /* rendezvous condition */
    int FirstReaderCondition;
    RequestQueue ReadRequestQueue;

    int ReaderCount;
    WS_ReaderInfo *Readers;
    char *Filename;
    int GlobalOpRequired;

    /* writer side marshal info */
    void *MarshalData;
    size_t MetadataSize;
    void *M; // building metadata block
    size_t DataSize;
    void *D; // building data block
    FFSFormatList PreviousFormats;

    /* READER-SIDE FIELDS */
    struct _TimestepMetadataList *Timesteps;
    int WriterCohortSize;
    int *Peers;
    CP_PeerConnection *ConnectionsToWriter;
    enum StreamStatus Status;
    int FinalTimestep;
    int CurrentWorkingTimestep;
    SstFullMetadata CurrentMetadata;
    struct _SstParams *WriterConfigParams;
    void *ParamsBlock;

    /* reader side marshal info */
    FFSContext ReaderFFSContext;
    VarSetupUpcallFunc VarSetupUpcall;
    ArraySetupUpcallFunc ArraySetupUpcall;
    void *SetupUpcallReader;

    void *ReaderMarshalData;
};

/*
 * This is the baseline contact information for each reader-side rank.
 * It will be gathered and provided to writer ranks
 */
typedef struct _CP_ReaderInitInfo
{
    char *ContactInfo;
    void *ReaderID;
} * CP_ReaderInitInfo;

/*
 * This is the structure that holds reader_side CP and DP contact info for a
 * single rank.
 * This is gathered on reader side.
 */
struct _CP_DP_PairInfo
{
    void **CP_Info;
    void **DP_Info;
};

/*
 * This is the structure that holds information about FFSformats for data
 * and metadata.  We transmit format ID and server reps from writers (who
 * encode) to readers (who decode) so that we don't need a third party
 * format server.
 */
struct FFSFormatBlock
{
    char *FormatServerRep;
    int FormatServerRepLen;
    char *FormatIDRep;
    int FormatIDRepLen;
    struct FFSFormatBlock *Next;
};

/*
 * This is the structure that holds local metadata and the DP info related to
 * it.
 * This is gathered on writer side before distribution to readers.
 */
struct _MetadataPlusDPInfo
{
    int RequestGlobalOp;
    SstData Metadata;
    FFSFormatList Formats;
    void *DP_TimestepInfo;
};

/*
 * Reader register messages are sent from reader rank 0 to writer rank 0
 * They contain basic info, plus contact information for each reader rank
 */
struct _ReaderRegisterMsg
{
    void *WriterFile;
    int WriterResponseCondition;
    int ReaderCohortSize;
    CP_ReaderInitInfo *CP_ReaderInfo;
    void **DP_ReaderInfo;
};

/*
 * This is the consolidated reader contact info structure that is used to
 * diseminate full reader contact information to all writer ranks
 */
typedef struct _CombinedReaderInfo
{
    int ReaderCohortSize;
    CP_ReaderInitInfo *CP_ReaderInfo;
    void **DP_ReaderInfo;
} * reader_data_t;

/*
 * This is the baseline contact information for each writer-side rank.
 * It will be gathered and provided to reader ranks
 */
typedef struct _CP_WriterInitInfo
{
    char *ContactInfo;
    void *WriterID;
} * CP_WriterInitInfo;

/*
 * Writer response messages from writer rank 0 to reader rank 0 after the
 * initial contact request.
 * They contain basic info, plus contact information for each reader rank
 */
struct _WriterResponseMsg
{
    int WriterResponseCondition;
    int WriterCohortSize;
    struct _SstParams *WriterConfigParams;
    size_t NextStepNumber;
    CP_WriterInitInfo *CP_WriterInfo;
    void **DP_WriterInfo;
};

/*
 * The ReaderActivate message informs the writer that this reader is now ready
 * to receive data/timesteps.
 * One is sent to each writer rank.
 */
struct _ReaderActivateMsg
{
    void *WSR_Stream;
};

/*
 * The timestepMetadata message carries the metadata from all writer ranks.
 * One is sent to each reader.
 */
typedef struct _TimestepMetadataMsg
{
    void *RS_Stream;
    int Timestep;
    int CohortSize;
    FFSFormatList Formats;
    SstData *Metadata;
    void **DP_TimestepInfo;
} * TSMetadataMsg;

/*
 * The ReleaseTimestep message informs the writers that this reader is done with
 * a particular timestep.
 * One is sent to each writer rank.
 */
struct _ReleaseTimestepMsg
{
    void *WSR_Stream;
    int Timestep;
};

/*
 * The WriterClose message informs the readers that the writer is beginning an
 * orderly shutdown
 * of the stream.  Data will still be served, but no new timesteps will be
 * forthcoming.
 * One is sent to each reader rank.
 */
typedef struct _WriterCloseMsg
{
    void *RS_Stream;
    int FinalTimestep;
} * WriterCloseMsg;

/*
 * The ReaderClose message informs the readers that the reader is beginning an
 * orderly shutdown of the stream.  One is sent to each writer rank.
 */
typedef struct _ReaderCloseMsg
{
    void *WSR_Stream;
} * ReaderCloseMsg;

/*
 * This is the consolidated writer contact info structure that is used to
 * diseminate full writer contact information to all reader ranks
 */
typedef struct _CombinedWriterInfo
{
    int WriterCohortSize;
    SstParams WriterConfigParams;
    size_t StartingStepNumber;
    CP_WriterInitInfo *CP_WriterInfo;
    void **DP_WriterInfo;
} * writer_data_t;

typedef struct _MetadataPlusDPInfo *MetadataPlusDPInfo;

extern atom_t CM_TRANSPORT_ATOM;

void CP_validateParams(SstStream stream, SstParams Params, int Writer);
extern CP_GlobalInfo CP_getCPInfo(CP_DP_Interface DPInfo);
extern char *CP_GetContactString(SstStream s);
extern SstStream CP_newStream();
extern void SstInternalProvideTimestep(SstStream s, SstData LocalMetadata,
                                       SstData Data, long Timestep,
                                       FFSFormatList Formats,
                                       DataFreeFunc FreeTimestep,
                                       void *FreeClientData);

void **CP_consolidateDataToRankZero(SstStream stream, void *local_info,
                                    FFSTypeHandle type, void **ret_data_block);
void **CP_consolidateDataToAll(SstStream stream, void *local_info,
                               FFSTypeHandle type, void **ret_data_block);
void *CP_distributeDataFromRankZero(SstStream stream, void *root_info,
                                    FFSTypeHandle type, void **ret_data_block);
extern void CP_ReaderRegisterHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_data,
                                     attr_list attrs);
extern void CP_WriterResponseHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_data,
                                     attr_list attrs);
extern void CP_ReaderActivateHandler(CManager cm, CMConnection conn,
                                     void *msg_v, void *client_data,
                                     attr_list attrs);
extern void CP_TimestepMetadataHandler(CManager cm, CMConnection conn,
                                       void *msg_v, void *client_data,
                                       attr_list attrs);
extern void CP_ReleaseTimestepHandler(CManager cm, CMConnection conn,
                                      void *msg_v, void *client_data,
                                      attr_list attrs);
extern void CP_WriterCloseHandler(CManager cm, CMConnection conn, void *msg_v,
                                  void *client_data, attr_list attrs);
extern void CP_ReaderCloseHandler(CManager cm, CMConnection conn, void *msg_v,
                                  void *client_data, attr_list attrs);

extern void FFSMarshalInstallMetadata(SstStream Stream, TSMetadataMsg MetaData);
extern void FFSClearTimestepData(SstStream Stream);
extern void FFSFreeMarshalData(SstStream Stream);
extern int *setupPeerArray(int MySize, int MyRank, int PeerSize);
extern void AddToLastCallFreeList(void *Block);
extern void CP_verbose(SstStream Stream, char *Format, ...);
extern void CP_error(SstStream Stream, char *Format, ...);
extern struct _CP_Services Svcs;
