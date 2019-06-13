#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "adios2/ADIOSConfig.h"
#include <atl.h>
#include <ffs.h>


/*
 *  metadata and typedefs are tentative and may come from ADIOS2 constructors.
 */
typedef struct _SstFullMetadata *SstFullMetadata;
typedef struct _SstData *SstData;

typedef enum
{
    SstSuccess,
    SstEndOfStream,
    SstFatalError,
    SstTimeout
} SstStatusValue;

/* The SST version of enum class StepMode in ADIOSTypes.h */
typedef enum
{
    SstAppend, // writer modes ignored in SST
    SstUpdate, // writer modes ignored in SST
    SstNextAvailable,
    SstLatestAvailable // reader advance mode
} SstStepMode;

/*
 * Struct that represents statistics tracked by SST
 */
typedef struct _SstStats
{
    double OpenTimeSecs;
    double CloseTimeSecs;
    double ValidTimeSecs;
    size_t BytesTransferred;
} * SstStats;

typedef enum
{
    SstMarshalFFS,
    SstMarshalBP
} SstMarshalMethod;

typedef enum
{
    SstCPCommMin,
    SstCPCommPeer
} SstCPCommPattern;

typedef enum
{
    SstQueueFullBlock = 0,
    SstQueueFullDiscard = 1
} SstQueueFullPolicy;

typedef enum
{
    SstCompressNone = 0,
    SstCompressZFP = 1
} SstCompressionMethod;


struct _SstFullMetadata
{
    int WriterCohortSize;
    struct _SstData **WriterMetadata;
    void **DP_TimestepInfo;
};

struct _SstData
{
    size_t DataSize;
    char *block;
};

struct _SstBlock
{
    size_t BlockSize;
    char *BlockData;
};


typedef enum
{
    SstRegisterFile,
    SstRegisterScreen,
    SstRegisterCloud
} SstRegistrationMethod;


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

typedef struct FFSFormatBlock *FFSFormatList;


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
    SstData Metadata;
    SstData AttributeData;
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
    void *RankZeroID;
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
    size_t NextStepNumber;
    CP_WriterInitInfo *CP_WriterInfo;
    void **DP_WriterInfo;
};

/*
 * The timestepMetadata message carries the metadata from all writer ranks.
 * One is sent to each reader.
 */
typedef struct _PeerSetupMsg
{
    void *RS_Stream;
    int WriterRank;
    int WriterCohortSize;
} * PeerSetupMsg;

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
 * One is sent to each reader in peer mode, between rank 0's in min mode.
 */
typedef struct _TimestepMetadataMsg
{
    void *RS_Stream;
    int Timestep;
    int CohortSize;
    FFSFormatList Formats;
    SstData Metadata;
    SstData AttributeData;
    void **DP_TimestepInfo;
} * TSMetadataMsg;

/*
 * The timestepMetadataDistribution message carries the metadata from rank 0 to
 * all reader ranks in min ode.
 */
typedef struct _TimestepMetadataDistributionMsg
{
    int ReturnValue;
    TSMetadataMsg TSmsg;
} * TSMetadataDistributionMsg;

/*
 * This is the structure that holds local metadata and the DP info related to
 * it.
 * This is gathered on writer side before distribution to readers.
 */

typedef struct _ReleaseRec
{
    long Timestep;
    void *Reader;
} * ReleaseRecPtr;

typedef struct _ReturnMetadataInfo
{
//    int DiscardThisTimestep;
//    int PendingReaderCount;
#ifdef RELEASECOUNT
    int ReleaseCount;
    ReleaseRecPtr ReleaseList;
#endif
    int ReaderCount;
    int *ReaderStatus;
    struct _TimestepMetadataMsg Msg;
} * ReturnMetadataInfo;

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
    size_t StartingStepNumber;
    CP_WriterInitInfo *CP_WriterInfo;
    void **DP_WriterInfo;
} * writer_data_t;

typedef struct _MetadataPlusDPInfo *MetadataPlusDPInfo;

extern atom_t CM_TRANSPORT_ATOM;

void *CP_distributeDataFromRankZero(FFSContext ffs_c, void *root_info,
                                    FFSTypeHandle type, void **ret_data_block);

static FMField CP_ReaderInitList[] = {
    {"ContactInfo", "string", sizeof(char *),
     FMOffset(CP_ReaderInitInfo, ContactInfo)},
    {"reader_ID", "integer", sizeof(void *),
     FMOffset(CP_ReaderInitInfo, ReaderID)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_ReaderInitStructs[] = {
    {"cp_reader", CP_ReaderInitList, sizeof(struct _CP_ReaderInitInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_WriterInitList[] = {
    {"ContactInfo", "string", sizeof(char *),
     FMOffset(CP_WriterInitInfo, ContactInfo)},
    {"WriterID", "integer", sizeof(void *),
     FMOffset(CP_WriterInitInfo, WriterID)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_WriterInitStructs[] = {
    {"cp_writer", CP_WriterInitList, sizeof(struct _CP_WriterInitInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_DP_PairList[] = {
    {"CP_Info", "*CP_STRUCT", 0, FMOffset(struct _CP_DP_PairInfo *, CP_Info)},
    {"DP_Info", "*DP_STRUCT", 0, FMOffset(struct _CP_DP_PairInfo *, DP_Info)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_DP_PairStructs[] = {
    {"CP_DP_pair", CP_DP_PairList, sizeof(struct _CP_DP_PairInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMStructDescRec CP_DP_WriterPairStructs[] = {
    {"CP_DP_WriterPair", CP_DP_PairList, sizeof(struct _CP_DP_PairInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_DP_ArrayReaderList[] = {
    {"ReaderCohortSize", "integer", sizeof(int),
     FMOffset(struct _CombinedReaderInfo *, ReaderCohortSize)},
    {"CP_ReaderInfo", "(*CP_STRUCT)[ReaderCohortSize]",
     sizeof(struct _CP_ReaderInitInfo),
     FMOffset(struct _CombinedReaderInfo *, CP_ReaderInfo)},
    {"DP_ReaderInfo", "(*DP_STRUCT)[ReaderCohortSize]", 0,
     FMOffset(struct _CombinedReaderInfo *, DP_ReaderInfo)},
    {"RankZeroID", "integer", sizeof(void *),
     FMOffset(struct _CombinedReaderInfo *, RankZeroID)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_DP_ReaderArrayStructs[] = {
    {"CombinedReaderInfo", CP_DP_ArrayReaderList,
     sizeof(struct _CombinedReaderInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_DP_ArrayWriterList[] = {
    {"WriterCohortSize", "integer", sizeof(int),
     FMOffset(struct _CombinedWriterInfo *, WriterCohortSize)},
    {"StartingStepNumber", "integer", sizeof(size_t),
     FMOffset(struct _CombinedWriterInfo *, StartingStepNumber)},
    {"CP_WriterInfo", "(*CP_STRUCT)[WriterCohortSize]",
     sizeof(struct _CP_WriterInitInfo),
     FMOffset(struct _CombinedWriterInfo *, CP_WriterInfo)},
    {"DP_WriterInfo", "(*DP_STRUCT)[WriterCohortSize]", 0,
     FMOffset(struct _CombinedWriterInfo *, DP_WriterInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_DP_WriterArrayStructs[] = {
    {"CombinedWriterInfo", CP_DP_ArrayWriterList,
     sizeof(struct _CombinedWriterInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_ReaderRegisterList[] = {
    {"writer_ID", "integer", sizeof(void *),
     FMOffset(struct _ReaderRegisterMsg *, WriterFile)},
    {"writer_response_condition", "integer", sizeof(int),
     FMOffset(struct _ReaderRegisterMsg *, WriterResponseCondition)},
    {"ReaderCohortSize", "integer", sizeof(int),
     FMOffset(struct _ReaderRegisterMsg *, ReaderCohortSize)},
    {"CP_ReaderInfo", "(*CP_STRUCT)[ReaderCohortSize]",
     sizeof(struct _CP_ReaderInitInfo),
     FMOffset(struct _ReaderRegisterMsg *, CP_ReaderInfo)},
    {"DP_ReaderInfo", "(*DP_STRUCT)[ReaderCohortSize]", 0,
     FMOffset(struct _ReaderRegisterMsg *, DP_ReaderInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_ReaderRegisterStructs[] = {
    {"ReaderRegister", CP_ReaderRegisterList, sizeof(struct _ReaderRegisterMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_WriterResponseList[] = {
    {"WriterResponseCondition", "integer", sizeof(int),
     FMOffset(struct _WriterResponseMsg *, WriterResponseCondition)},
    {"WriterCohortSize", "integer", sizeof(int),
     FMOffset(struct _WriterResponseMsg *, WriterCohortSize)},
    {"NextStepNumber", "integer", sizeof(size_t),
     FMOffset(struct _WriterResponseMsg *, NextStepNumber)},
    {"cp_WriterInfo", "(*CP_STRUCT)[WriterCohortSize]",
     sizeof(struct _CP_WriterInitInfo),
     FMOffset(struct _WriterResponseMsg *, CP_WriterInfo)},
    {"dp_WriterInfo", "(*DP_STRUCT)[WriterCohortSize]", 0,
     FMOffset(struct _WriterResponseMsg *, DP_WriterInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_WriterResponseStructs[] = {
    {"WriterResponse", CP_WriterResponseList, sizeof(struct _WriterResponseMsg),
     NULL},
    {NULL, NULL, 0, NULL}};

static FMField MetaDataPlusDPInfoList[] = {
    {"Metadata", "*SstBlock", sizeof(struct _SstBlock),
     FMOffset(struct _MetadataPlusDPInfo *, Metadata)},
    {"AttributeData", "*SstBlock", sizeof(struct _SstBlock),
     FMOffset(struct _MetadataPlusDPInfo *, AttributeData)},
    {"Formats", "*FFSFormatBlock", sizeof(struct FFSFormatBlock),
     FMOffset(struct _MetadataPlusDPInfo *, Formats)},
    {"DP_TimestepInfo", "*DP_STRUCT", 0,
     FMOffset(struct _MetadataPlusDPInfo *, DP_TimestepInfo)},
    {NULL, NULL, 0, 0}};

static FMField FFSFormatBlockList[] = {
    {"FormatServerRep", "char[FormatServerRepLen]", 1,
     FMOffset(struct FFSFormatBlock *, FormatServerRep)},
    {"FormatServerRepLen", "integer", sizeof(int),
     FMOffset(struct FFSFormatBlock *, FormatServerRepLen)},
    {"FormatIDRep", "char[FormatIDRepLen]", 1,
     FMOffset(struct FFSFormatBlock *, FormatIDRep)},
    {"FormatIDRepLen", "integer", sizeof(int),
     FMOffset(struct FFSFormatBlock *, FormatIDRepLen)},
    {"Next", "*FFSFormatBlock", sizeof(struct FFSFormatBlock),
     FMOffset(struct FFSFormatBlock *, Next)},
    {NULL, NULL, 0, 0}};

static FMField SstBlockList[] = {{"BlockSize", "integer", sizeof(size_t),
                                  FMOffset(struct _SstBlock *, BlockSize)},
                                 {"BlockData", "char[BlockSize]", 1,
                                  FMOffset(struct _SstBlock *, BlockData)},
                                 {NULL, NULL, 0, 0}};

static FMStructDescRec MetaDataPlusDPInfoStructs[] = {
    {"MetaDataPlusDPInfo", MetaDataPlusDPInfoList,
     sizeof(struct _MetadataPlusDPInfo), NULL},
    {"FFSFormatBlock", FFSFormatBlockList, sizeof(struct FFSFormatBlock), NULL},
    {"SstBlock", SstBlockList, sizeof(struct _SstBlock), NULL},
    {NULL, NULL, 0, NULL}};

static FMField TimestepMetadataList[] = {
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(struct _TimestepMetadataMsg *, RS_Stream)},
    {"timestep", "integer", sizeof(int),
     FMOffset(struct _TimestepMetadataMsg *, Timestep)},
    {"cohort_size", "integer", sizeof(int),
     FMOffset(struct _TimestepMetadataMsg *, CohortSize)},
    {"formats", "*FFSFormatBlock", sizeof(struct FFSFormatBlock),
     FMOffset(struct _TimestepMetadataMsg *, Formats)},
    {"metadata", "SstBlock[cohort_size]", sizeof(struct _SstBlock),
     FMOffset(struct _TimestepMetadataMsg *, Metadata)},
    {"attribute_data", "SstBlock[cohort_size]", sizeof(struct _SstBlock),
     FMOffset(struct _TimestepMetadataMsg *, AttributeData)},
    {"TP_TimestepInfo", "(*DP_STRUCT)[cohort_size]", 0,
     FMOffset(struct _TimestepMetadataMsg *, DP_TimestepInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec TimestepMetadataStructs[] = {
    {"timestepMetadata", TimestepMetadataList,
     sizeof(struct _TimestepMetadataMsg), NULL},
    {"FFSFormatBlock", FFSFormatBlockList, sizeof(struct FFSFormatBlock), NULL},
    {"SstBlock", SstBlockList, sizeof(struct _SstBlock), NULL},
    {NULL, NULL, 0, NULL}};

static FMField TimestepMetadataDistributionList[] = {
    {"ReturnValue", "integer", sizeof(int),
     FMOffset(struct _TimestepMetadataDistributionMsg *, ReturnValue)},
    {"TSmsg", "*timestepMetadata", sizeof(struct _TimestepMetadataMsg),
     FMOffset(struct _TimestepMetadataDistributionMsg *, TSmsg)},
    {NULL, NULL, 0, 0}};

static FMField ReleaseRecList[] = {{"Timestep", "integer", sizeof(long),
                                    FMOffset(struct _ReleaseRec *, Timestep)},
                                   {"Reader", "integer", sizeof(void *),
                                    FMOffset(struct _ReleaseRec *, Reader)},
                                   {NULL, NULL, 0, 0}};

static FMField ReturnMetadataInfoList[] = {
//    {"DiscardThisTimestep", "integer", sizeof(int),
//     FMOffset(struct _ReturnMetadataInfo *, DiscardThisTimestep)},
//    {"PendingReaderCount", "integer", sizeof(int),
//     FMOffset(struct _ReturnMetadataInfo *, PendingReaderCount)},
#ifdef RELEASECOUNT
    {"ReleaseCount", "integer", sizeof(int),
     FMOffset(struct _ReturnMetadataInfo *, ReleaseCount)},
    {"ReleaseList", "ReleaseRec[ReleaseCount]", sizeof(struct _ReleaseRec),
     FMOffset(struct _ReturnMetadataInfo *, ReleaseList)},
#endif
    {"ReaderCount", "integer", sizeof(int),
     FMOffset(struct _ReturnMetadataInfo *, ReaderCount)},
    {"ReaderStatus", "integer[ReaderCount]", sizeof(int),
     FMOffset(struct _ReturnMetadataInfo *, ReaderStatus)},
    {"Msg", "timestepMetadata", sizeof(struct _TimestepMetadataMsg),
     FMOffset(struct _ReturnMetadataInfo *, Msg)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec TimestepMetadataDistributionStructs[] = {
    {"TimestepDistribution", TimestepMetadataDistributionList,
     sizeof(struct _TimestepMetadataDistributionMsg), NULL},
    {"timestepMetadata", TimestepMetadataList,
     sizeof(struct _TimestepMetadataMsg), NULL},
    {"FFSFormatBlock", FFSFormatBlockList, sizeof(struct FFSFormatBlock), NULL},
    {"SstBlock", SstBlockList, sizeof(struct _SstBlock), NULL},
    {NULL, NULL, 0, NULL}};

static FMStructDescRec ReturnMetadataInfoStructs[] = {
    {"ReturnMetadataInfo", ReturnMetadataInfoList,
     sizeof(struct _TimestepMetadataDistributionMsg), NULL},
    {"ReleaseRec", ReleaseRecList, sizeof(struct _ReleaseRec), NULL},
    {"timestepMetadata", TimestepMetadataList,
     sizeof(struct _TimestepMetadataMsg), NULL},
    {"FFSFormatBlock", FFSFormatBlockList, sizeof(struct FFSFormatBlock), NULL},
    {"SstBlock", SstBlockList, sizeof(struct _SstBlock), NULL},
    {NULL, NULL, 0, NULL}};

static FMField ReleaseTimestepList[] = {
    {"WSR_Stream", "integer", sizeof(void *),
     FMOffset(struct _ReleaseTimestepMsg *, WSR_Stream)},
    {"Timestep", "integer", sizeof(int),
     FMOffset(struct _ReleaseTimestepMsg *, Timestep)},
    {NULL, NULL, 0, 0}};

static FMField PeerSetupList[] = {
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(struct _PeerSetupMsg *, RS_Stream)},
    {"WriterRank", "integer", sizeof(int),
     FMOffset(struct _PeerSetupMsg *, WriterRank)},
    {"WriterCohortSize", "integer", sizeof(int),
     FMOffset(struct _PeerSetupMsg *, WriterCohortSize)},
    {NULL, NULL, 0, 0}};

static FMField ReaderActivateList[] = {
    {"WSR_Stream", "integer", sizeof(void *),
     FMOffset(struct _ReaderActivateMsg *, WSR_Stream)},
    {NULL, NULL, 0, 0}};

static FMField WriterCloseList[] = {
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(struct _WriterCloseMsg *, RS_Stream)},
    {"FinalTimestep", "integer", sizeof(int),
     FMOffset(struct _WriterCloseMsg *, FinalTimestep)},
    {NULL, NULL, 0, 0}};

static FMField ReaderCloseList[] = {
    {"WSR_Stream", "integer", sizeof(void *),
     FMOffset(struct _ReaderCloseMsg *, WSR_Stream)},
    {NULL, NULL, 0, 0}};

static void replaceFormatNameInFieldList(FMStructDescList l, char *orig,
                                         char *repl, int repl_size)
{
    int i = 0;
    while (l[i].format_name)
    {
        int j = 0;
        while (l[i].field_list[j].field_name)
        {
            char *loc;
            if ((loc = strstr(l[i].field_list[j].field_type, orig)))
            {
                if (repl)
                {
                    /* replace 'orig' with 'repl' */
                    char *old = (char *)l[i].field_list[j].field_type;
                    char *new =
                        malloc(strlen(old) - strlen(orig) + strlen(repl) + 1);
                    strncpy(new, old, loc - old);
                    new[loc - old] = 0;
                    strcat(new, repl);
                    strcat(new, loc + strlen(orig));
                    free(old);
                    l[i].field_list[j].field_type = new;
                    l[i].field_list[j].field_size = repl_size;
                }
                else
                {
                    /* remove list item with 'orig'  Move higher elements down 1
                     */
                    int index = j;
                    free((char *)l[i].field_list[j].field_name);
                    free((char *)l[i].field_list[j].field_type);
                    while (l[i].field_list[index].field_name != NULL)
                    {
                        l[i].field_list[index] = l[i].field_list[index + 1];
                    }
                    j--; /* we've replaced this element, make sure we process
                            the one we replaced it with */
                }
            }
            j++;
        }
        i++;
    }
}

/*
 * generated a combined FMStructDescList from separate top-level, cp and dp
 * formats
 * the format names/sizes "CP_STRUCT" and "DP_STRUCT" used in top-level field
 * lists are replaced by
 * the actual names/sizes provided.
 */
static FMStructDescList combineCpDpFormats(FMStructDescList top,
                                           FMStructDescList cp,
                                           FMStructDescList dp)
{
    int i = 0, topCount = 0, cpCount = 0, dpCount = 0;
    FMStructDescList CombinedFormats = FMcopy_struct_list(top);

    i = 0;
    while (top[i++].format_name)
        topCount++;

    i = 0;
    while (cp && cp[i++].format_name)
        cpCount++;

    i = 0;
    while (dp && dp[i++].format_name)
        dpCount++;

    CombinedFormats =
        realloc(CombinedFormats, sizeof(CombinedFormats[0]) *
                                     (topCount + cpCount + dpCount + 1));
    for (i = 0; i < cpCount; i++)
    {
        CombinedFormats[topCount + i].format_name = strdup(cp[i].format_name);
        CombinedFormats[topCount + i].field_list =
            copy_field_list(cp[i].field_list);
        CombinedFormats[topCount + i].struct_size = cp[i].struct_size;
        CombinedFormats[topCount + i].opt_info = NULL;
    }

    for (i = 0; i < dpCount; i++)
    {
        CombinedFormats[topCount + cpCount + i].format_name =
            strdup(dp[i].format_name);
        CombinedFormats[topCount + cpCount + i].field_list =
            copy_field_list(dp[i].field_list);
        CombinedFormats[topCount + cpCount + i].struct_size = dp[i].struct_size;
        CombinedFormats[topCount + cpCount + i].opt_info = NULL;
    }
    CombinedFormats[topCount + cpCount + dpCount].format_name = NULL;
    CombinedFormats[topCount + cpCount + dpCount].field_list = NULL;
    CombinedFormats[topCount + cpCount + dpCount].struct_size = 0;
    CombinedFormats[topCount + cpCount + dpCount].opt_info = NULL;

    replaceFormatNameInFieldList(CombinedFormats, "CP_STRUCT",
                                 cp ? cp[0].format_name : NULL,
                                 cp ? cp[0].struct_size : 0);
    replaceFormatNameInFieldList(CombinedFormats, "DP_STRUCT",
                                 dp ? dp[0].format_name : NULL,
                                 dp ? dp[0].struct_size : 0);
    return CombinedFormats;
}

static int verbose = 0;
void *CP_distributeDataFromRankZero(FFSContext ffs_c, void *root_info,
                                    FFSTypeHandle Type, void **RetDataBlock)
{
    int DataSize;
    char *Buffer;
    void *RetVal;

    FFSBuffer Buf = create_FFSBuffer();
    char *tmp =
        FFSencode(Buf, FMFormat_of_original(Type), root_info, &DataSize);
    if (verbose) {
        printf("Provided data is : \n");
        FMdump_data(FMFormat_of_original(Type), root_info, 1024000);
    }
    Buffer = malloc(DataSize);
    memcpy(Buffer, tmp, DataSize);
    free_FFSBuffer(Buf);

    FFSContext context = ffs_c;
    // FFSTypeHandle ffs_type = FFSTypeHandle_from_encode(context, Buffer);
    if (verbose) printf("BUFFER IS AT %p, DataSize is %d (0x%x), End is %p\n", Buffer, DataSize, DataSize, Buffer+DataSize);

    FFSdecode_in_place(context, Buffer, &RetVal);
    if (verbose) {
        printf("Decode is : \n");
        FMdump_data(FMFormat_of_original(Type), RetVal, 1024000);
    }
    *RetDataBlock = Buffer;
    return RetVal;
}


FFSTypeHandle ReturnMetadataInfoFormat;

static void doFormatRegistration(FFSContext ffs_c, FMContext fm_c)
{
    FMStructDescList CombinedMetadataStructs, CombinedTimestepMetadataStructs;
    FMFormat f;

    /*gse*/ CombinedMetadataStructs = combineCpDpFormats(
        ReturnMetadataInfoStructs, NULL, NULL /* DPInfo->TimestepInfoFormats */);
    f = FMregister_data_format(fm_c, CombinedMetadataStructs);
    ReturnMetadataInfoFormat =
        FFSTypeHandle_by_index(ffs_c, FMformat_index(f));
    FFSset_fixed_target(ffs_c, CombinedMetadataStructs);

}

static void FillMetadataMsg(int CohortSize, struct _TimestepMetadataMsg *Msg,
                            MetadataPlusDPInfo *pointers)
{
    FFSFormatList XmitFormats = NULL;

    /* build the Metadata Msg */
    Msg->CohortSize = CohortSize;
    Msg->CohortSize = 1;
    Msg->Timestep = 3;
    Msg->Formats = NULL;


    /* separate metadata and DP_info to separate arrays */
    Msg->Metadata = malloc(CohortSize * sizeof(Msg->Metadata[0]));
    Msg->Metadata[0].DataSize = 1392;
    Msg->Metadata[0].block = calloc(1,     Msg->Metadata[0].DataSize);
    *(int*)Msg->Metadata[0].block = 0xdeadbeef;
    Msg->AttributeData = malloc(CohortSize * sizeof(Msg->Metadata[0]));
    Msg->AttributeData[0].DataSize = 0;
    Msg->AttributeData[0].block = NULL;
    Msg->DP_TimestepInfo =
        malloc(CohortSize * sizeof(Msg->DP_TimestepInfo[0]));

    Msg->DP_TimestepInfo = NULL;


}

int
main(int argc, char **argv)
{
    FFSContext ffs_c;
    FMContext fm_c;
    void *RetDataBlock;
    ReturnMetadataInfo ReturnData;
    struct _ReturnMetadataInfo TimestepMetaData;
    fm_c = create_local_FMcontext();
    ffs_c = create_FFSContext_FM(fm_c);

    if ((argc > 1) && (strcmp(argv[1], "-v") == 0)) verbose++;
                             
    doFormatRegistration(ffs_c, fm_c);

    struct _MetadataPlusDPInfo * pointers = malloc(sizeof(*pointers));

    memset(&TimestepMetaData, 0, sizeof(TimestepMetaData));
#ifdef RELEASECOUNT
    TimestepMetaData.ReleaseCount = 0;
    TimestepMetaData.ReleaseList = NULL;
#endif
    TimestepMetaData.ReaderCount = 0;
    TimestepMetaData.ReaderStatus = malloc(sizeof(int));
    for (int i = 0; i < TimestepMetaData.ReaderCount; i++)
    {
        TimestepMetaData.ReaderStatus[i] = 1;
        }
    FillMetadataMsg(1 /* CohortSize */, &TimestepMetaData.Msg, &pointers);

    if (verbose) {
        printf("Before, TimestepMetaData.Metadata[0].DataSize = %ld  block %p \n", TimestepMetaData.Msg.Metadata[0].DataSize, TimestepMetaData.Msg.Metadata[0].block);

        printf("&TimestepMetaData = %p\n", &TimestepMetaData);
        printf("TimestepMetaData.ReaderCount = %d, ReaderStatus = %p\n", TimestepMetaData.ReaderCount, TimestepMetaData.ReaderStatus);
        printf("&TimestepMetaData.Msg = %p\n", &TimestepMetaData.Msg);
        printf("TimestepMetaData.Msg.CohortSize = %d, Timestep = %d, Formats = %p\n", TimestepMetaData.Msg.CohortSize, TimestepMetaData.Msg.Timestep, TimestepMetaData.Msg.Formats);
        printf("TimestepMetaData.Msg.Metadata = %p\n", TimestepMetaData.Msg.Metadata);
        if (TimestepMetaData.Msg.Metadata)
            printf("TimestepMetaData.Msg.Metadata[0].block = %p, .size = %ld\n", TimestepMetaData.Msg.Metadata[0].block, TimestepMetaData.Msg.Metadata[0].DataSize);
    }
    ReturnData = CP_distributeDataFromRankZero(ffs_c, &TimestepMetaData,
                                             ReturnMetadataInfoFormat, &RetDataBlock);

    if (verbose) {
        printf("ReturnData = %p\n", ReturnData);
        printf("ReturnData->ReaderCount = %d, ReaderStatus = %p\n", ReturnData->ReaderCount, ReturnData->ReaderStatus);
        printf("&ReturnData->Msg = %p\n", &ReturnData->Msg);
        printf("ReturnData->Msg.CohortSize = %d, Timestep = %d, Formats = %p\n", ReturnData->Msg.CohortSize, ReturnData->Msg.Timestep, ReturnData->Msg.Formats);
        printf("ReturnData->Msg.Metadata = %p\n", ReturnData->Msg.Metadata);
        if (ReturnData->Msg.Metadata)
            printf("ReturnData->Msg.Metadata[0].block = %p, .size = %ld\n", ReturnData->Msg.Metadata[0].block, ReturnData->Msg.Metadata[0].DataSize);
        printf("After, ReturnData.Metadata[0](%p).DataSize = %ld  block %p \n", ReturnData->Msg.Metadata, ReturnData->Msg.Metadata[0].DataSize, ReturnData->Msg.Metadata[0].block);
        printf("After, ReturnData.Metadata[0].block[0] = 0x%0x \n", *(int*)ReturnData->Msg.Metadata[0].block);
    }
    if(*(int*)ReturnData->Msg.Metadata[0].block != 0xdeadbeef) {fprintf(stderr, "BAD VALUE\n"); exit(1);}
}
