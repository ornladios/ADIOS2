#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <atl.h>
#include <evpath.h>
#include <mpi.h>

#include "sst.h"

#include "cp_internal.h"

void CP_validateParams(SstStream Stream, SstParams Params, int Writer)
{
    if (Params->RendezvousReaderCount >= 0)
    {
        Stream->RendezvousReaderCount = Params->RendezvousReaderCount;
    }
    else
    {
        fprintf(stderr, "Invalid RendezvousReaderCount parameter value (%d) "
                        "for SST Stream %s\n",
                Params->RendezvousReaderCount, Stream->Filename);
    }
    if (Params->QueueLimit >= 0)
    {
        Stream->QueueLimit = Params->QueueLimit;
    }
    else
    {
        fprintf(stderr,
                "Invalid QueueLimit parameter value (%d) for SST Stream %s\n",
                Params->QueueLimit, Stream->Filename);
    }
    Stream->DiscardOnQueueFull = Params->DiscardOnQueueFull;
    Stream->RegistrationMethod = Params->RegistrationMethod;
    char *SelectedTransport = NULL;
    if (Params->DataTransport != NULL)
    {
        int i;
        SelectedTransport = malloc(strlen(Params->DataTransport) + 1);
        for (i = 0; Params->DataTransport[i] != 0; i++)
        {
            SelectedTransport[i] = tolower(Params->DataTransport[i]);
        }
        SelectedTransport[i] = 0;

        /* canonicalize SelectedTransport */
        if ((strcmp(SelectedTransport, "wan") == 0) ||
            (strcmp(SelectedTransport, "evpath") == 0))
        {
            Stream->DataTransport = strdup("evpath");
        }
        else if ((strcmp(SelectedTransport, "rdma") == 0) ||
                 (strcmp(SelectedTransport, "ib") == 0) ||
                 (strcmp(SelectedTransport, "fabric") == 0))
        {
            Stream->DataTransport = strdup("rdma");
        }
        else
        {
            fprintf(stderr,
                    "Error: Unknown value '%s' for DataTransport parameter.\n",
                    SelectedTransport);
            Stream->DataTransport = NULL;
        }
        free(SelectedTransport);
    }
    if (Params->DataTransport == NULL)
    {
        /* determine reasonable default, now "evpath" */
        Stream->DataTransport = strdup("evpath");
    }
}

static FMField CP_SstParamsList_RAW[] = {
#define declare_field(Param, Type, Typedecl, Default)                          \
    {#Param, #Typedecl, sizeof(Typedecl), FMOffset(struct _SstParams *, Param)},
    SST_FOREACH_PARAMETER_TYPE_4ARGS(declare_field)
#undef declare_field
        {NULL, NULL, 0, 0}};
static FMField *CP_SstParamsList = NULL;

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
    {NULL, NULL, 0, 0}};

static FMStructDescRec CP_DP_ReaderArrayStructs[] = {
    {"CombinedReaderInfo", CP_DP_ArrayReaderList,
     sizeof(struct _CombinedReaderInfo), NULL},
    {NULL, NULL, 0, NULL}};

static FMField CP_DP_ArrayWriterList[] = {
    {"WriterCohortSize", "integer", sizeof(int),
     FMOffset(struct _CombinedWriterInfo *, WriterCohortSize)},
    {"WriterConfigParams", "*SstParams", sizeof(struct _SstParams),
     FMOffset(struct _CombinedWriterInfo *, WriterConfigParams)},
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
    {"SstParams", NULL, sizeof(struct _SstParams), NULL},
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
    {"WriterConfigParams", "*SstParams", sizeof(struct _SstParams),
     FMOffset(struct _WriterResponseMsg *, WriterConfigParams)},
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
    {"SstParams", NULL, sizeof(struct _SstParams), NULL},
    {NULL, NULL, 0, NULL}};

static FMField MetaDataPlusDPInfoList[] = {
    {"RequestGlobalOp", "integer", sizeof(int),
     FMOffset(struct _MetadataPlusDPInfo *, RequestGlobalOp)},
    {"Metadata", "*SstBlock", sizeof(struct _SstBlock),
     FMOffset(struct _MetadataPlusDPInfo *, Metadata)},
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
    {"metadata", "(*SstBlock)[cohort_size]", sizeof(struct _SstBlock),
     FMOffset(struct _TimestepMetadataMsg *, Metadata)},
    {"TP_TimestepInfo", "(*DP_STRUCT)[cohort_size]", 0,
     FMOffset(struct _TimestepMetadataMsg *, DP_TimestepInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec TimestepMetadataStructs[] = {
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

void **CP_consolidateDataToRankZero(SstStream Stream, void *LocalInfo,
                                    FFSTypeHandle Type, void **RetDataBlock)
{
    FFSBuffer Buf = create_FFSBuffer();
    int DataSize;
    int *RecvCounts = NULL;
    char *Buffer;

    struct _CP_DP_init_info **Pointers = NULL;

    Buffer = FFSencode(Buf, FMFormat_of_original(Type), LocalInfo, &DataSize);

    if (Stream->Rank == 0)
    {
        RecvCounts = malloc(Stream->CohortSize * sizeof(int));
    }
    MPI_Gather(&DataSize, 1, MPI_INT, RecvCounts, 1, MPI_INT, 0,
               Stream->mpiComm);

    /*
     * Figure out the total length of block
     * and displacements for each rank
     */

    int *Displs = NULL;
    char *RecvBuffer = NULL;

    if (Stream->Rank == 0)
    {
        int TotalLen = 0;
        Displs = malloc(Stream->CohortSize * sizeof(int));

        Displs[0] = 0;
        TotalLen = (RecvCounts[0] + 7) & ~7;

        for (int i = 1; i < Stream->CohortSize; i++)
        {
            int RoundUp = (RecvCounts[i] + 7) & ~7;
            Displs[i] = TotalLen;
            TotalLen += RoundUp;
        }

        RecvBuffer = malloc(TotalLen * sizeof(char));
    }

    /*
     * Now we have the receive buffer, counts, and displacements, and
     * can gather the data
     */

    MPI_Gatherv(Buffer, DataSize, MPI_CHAR, RecvBuffer, RecvCounts, Displs,
                MPI_CHAR, 0, Stream->mpiComm);
    free_FFSBuffer(Buf);

    if (Stream->Rank == 0)
    {
        FFSContext context = Stream->CPInfo->ffs_c;
        //        FFSTypeHandle ffs_type = FFSTypeHandle_from_encode(context,
        //        RecvBuffer);

        int i;
        Pointers = malloc(Stream->CohortSize * sizeof(Pointers[0]));
        for (i = 0; i < Stream->CohortSize; i++)
        {
            FFSdecode_in_place(context, RecvBuffer + Displs[i],
                               (void **)&Pointers[i]);
            // printf("Decode for rank %d :\n", i);
            // FMdump_data(FMFormat_of_original(ffs_type), Pointers[i],
            // 1024000);
        }
        free(Displs);
        free(RecvCounts);
    }
    *RetDataBlock = RecvBuffer;
    return (void **)Pointers;
}

void *CP_distributeDataFromRankZero(SstStream Stream, void *root_info,
                                    FFSTypeHandle Type, void **RetDataBlock)
{
    int DataSize;
    char *Buffer;
    void *RetVal;

    if (Stream->Rank == 0)
    {
        FFSBuffer Buf = create_FFSBuffer();
        char *tmp =
            FFSencode(Buf, FMFormat_of_original(Type), root_info, &DataSize);
        MPI_Bcast(&DataSize, 1, MPI_INT, 0, Stream->mpiComm);
        MPI_Bcast(tmp, DataSize, MPI_CHAR, 0, Stream->mpiComm);
        Buffer = malloc(DataSize);
        memcpy(Buffer, tmp, DataSize);
        free_FFSBuffer(Buf);
    }
    else
    {
        MPI_Bcast(&DataSize, 1, MPI_INT, 0, Stream->mpiComm);
        Buffer = malloc(DataSize);
        MPI_Bcast(Buffer, DataSize, MPI_CHAR, 0, Stream->mpiComm);
    }

    FFSContext context = Stream->CPInfo->ffs_c;
    // FFSTypeHandle ffs_type = FFSTypeHandle_from_encode(context, Buffer);

    FFSdecode_in_place(context, Buffer, &RetVal);
    // printf("Decode for rank %d is : \n", Stream->rank);
    // FMdump_data(FMFormat_of_original(ffs_type), RetVal, 1024000);
    *RetDataBlock = Buffer;
    return RetVal;
}

void **CP_consolidateDataToAll(SstStream Stream, void *LocalInfo,
                               FFSTypeHandle Type, void **RetDataBlock)
{
    FFSBuffer Buf = create_FFSBuffer();
    int DataSize;
    int *RecvCounts = NULL;
    char *Buffer;

    struct _CP_DP_init_info **Pointers = NULL;

    Buffer = FFSencode(Buf, FMFormat_of_original(Type), LocalInfo, &DataSize);

    RecvCounts = malloc(Stream->CohortSize * sizeof(int));

    MPI_Allgather(&DataSize, 1, MPI_INT, RecvCounts, 1, MPI_INT,
                  Stream->mpiComm);

    /*
     * Figure out the total length of block
     * and displacements for each rank
     */

    int *Displs = NULL;
    char *RecvBuffer = NULL;
    int i;

    int TotalLen = 0;
    Displs = malloc(Stream->CohortSize * sizeof(int));

    Displs[0] = 0;
    TotalLen = (RecvCounts[0] + 7) & ~7;

    for (i = 1; i < Stream->CohortSize; i++)
    {
        int round_up = (RecvCounts[i] + 7) & ~7;
        Displs[i] = TotalLen;
        TotalLen += round_up;
    }

    RecvBuffer = malloc(TotalLen * sizeof(char));

    /*
     * Now we have the receive Buffer, counts, and displacements, and
     * can gather the data
     */

    MPI_Allgatherv(Buffer, DataSize, MPI_CHAR, RecvBuffer, RecvCounts, Displs,
                   MPI_CHAR, Stream->mpiComm);
    free_FFSBuffer(Buf);

    FFSContext context = Stream->CPInfo->ffs_c;

    Pointers = malloc(Stream->CohortSize * sizeof(Pointers[0]));
    for (i = 0; i < Stream->CohortSize; i++)
    {
        FFSdecode_in_place(context, RecvBuffer + Displs[i],
                           (void **)&Pointers[i]);
    }
    free(Displs);
    free(RecvCounts);

    *RetDataBlock = RecvBuffer;
    return (void **)Pointers;
}

atom_t CM_TRANSPORT_ATOM = 0;

static void initAtomList()
{
    if (CM_TRANSPORT_ATOM)
        return;

    CM_TRANSPORT_ATOM = attr_atom_from_string("CM_TRANSPORT");
}

static void AddCustomStruct(CP_GlobalInfo CPInfo, FMStructDescList Struct)
{
    CPInfo->CustomStructCount++;
    CPInfo->CustomStructList =
        realloc(CPInfo->CustomStructList,
                sizeof(FMStructDescList) * CPInfo->CustomStructCount);
    CPInfo->CustomStructList[CPInfo->CustomStructCount - 1] = Struct;
}

static void FreeCustomStructs(CP_GlobalInfo CPInfo)
{
    for (int i = 0; i < CPInfo->CustomStructCount; i++)
    {
        FMfree_struct_list(CPInfo->CustomStructList[i]);
    }
    free(CPInfo->CustomStructList);
}

static void doFormatRegistration(CP_GlobalInfo CPInfo, CP_DP_Interface DPInfo)
{
    FMStructDescList PerRankReaderStructs, FullReaderRegisterStructs,
        CombinedReaderStructs;
    FMStructDescList PerRankWriterStructs, FullWriterResponseStructs,
        CombinedWriterStructs;
    FMStructDescList CombinedMetadataStructs, CombinedTimestepMetadataStructs;
    FMFormat f;

    PerRankReaderStructs = combineCpDpFormats(
        CP_DP_PairStructs, CP_ReaderInitStructs, DPInfo->ReaderContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, PerRankReaderStructs);
    CPInfo->PerRankReaderInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, PerRankReaderStructs);
    AddCustomStruct(CPInfo, PerRankReaderStructs);

    FullReaderRegisterStructs =
        combineCpDpFormats(CP_ReaderRegisterStructs, CP_ReaderInitStructs,
                           DPInfo->ReaderContactFormats);
    CPInfo->ReaderRegisterFormat =
        CMregister_format(CPInfo->cm, FullReaderRegisterStructs);
    CMregister_handler(CPInfo->ReaderRegisterFormat, CP_ReaderRegisterHandler,
                       NULL);
    AddCustomStruct(CPInfo, FullReaderRegisterStructs);

    /*gse*/ CombinedReaderStructs =
        combineCpDpFormats(CP_DP_ReaderArrayStructs, CP_ReaderInitStructs,
                           DPInfo->ReaderContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, CombinedReaderStructs);
    CPInfo->CombinedReaderInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, CombinedReaderStructs);
    AddCustomStruct(CPInfo, CombinedReaderStructs);

    /*gse*/ PerRankWriterStructs =
        combineCpDpFormats(CP_DP_WriterPairStructs, CP_WriterInitStructs,
                           DPInfo->WriterContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, PerRankWriterStructs);
    CPInfo->PerRankWriterInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, PerRankWriterStructs);
    AddCustomStruct(CPInfo, PerRankWriterStructs);

    /*gse*/ FullWriterResponseStructs =
        combineCpDpFormats(CP_WriterResponseStructs, CP_WriterInitStructs,
                           DPInfo->WriterContactFormats);
    CPInfo->WriterResponseFormat =
        CMregister_format(CPInfo->cm, FullWriterResponseStructs);
    CMregister_handler(CPInfo->WriterResponseFormat, CP_WriterResponseHandler,
                       NULL);
    AddCustomStruct(CPInfo, FullWriterResponseStructs);

    /*gse*/ CombinedWriterStructs =
        combineCpDpFormats(CP_DP_WriterArrayStructs, CP_WriterInitStructs,
                           DPInfo->WriterContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, CombinedWriterStructs);
    CPInfo->CombinedWriterInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, CombinedWriterStructs);
    AddCustomStruct(CPInfo, CombinedWriterStructs);

    /*gse*/ CombinedMetadataStructs = combineCpDpFormats(
        MetaDataPlusDPInfoStructs, NULL, DPInfo->TimestepInfoFormats);
    f = FMregister_data_format(CPInfo->fm_c, CombinedMetadataStructs);
    CPInfo->PerRankMetadataFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, CombinedMetadataStructs);
    AddCustomStruct(CPInfo, CombinedMetadataStructs);

    /*gse*/ CombinedTimestepMetadataStructs = combineCpDpFormats(
        TimestepMetadataStructs, NULL, DPInfo->TimestepInfoFormats);
    CPInfo->DeliverTimestepMetadataFormat =
        CMregister_format(CPInfo->cm, CombinedTimestepMetadataStructs);
    CMregister_handler(CPInfo->DeliverTimestepMetadataFormat,
                       CP_TimestepMetadataHandler, NULL);
    AddCustomStruct(CPInfo, CombinedTimestepMetadataStructs);

    CPInfo->ReaderActivateFormat = CMregister_simple_format(
        CPInfo->cm, "ReaderActivate", ReaderActivateList,
        sizeof(struct _ReaderActivateMsg));
    CMregister_handler(CPInfo->ReaderActivateFormat, CP_ReaderActivateHandler,
                       NULL);
    CPInfo->ReleaseTimestepFormat = CMregister_simple_format(
        CPInfo->cm, "ReleaseTimestep", ReleaseTimestepList,
        sizeof(struct _ReleaseTimestepMsg));
    CMregister_handler(CPInfo->ReleaseTimestepFormat, CP_ReleaseTimestepHandler,
                       NULL);
    CPInfo->WriterCloseFormat =
        CMregister_simple_format(CPInfo->cm, "WriterClose", WriterCloseList,
                                 sizeof(struct _WriterCloseMsg));
    CMregister_handler(CPInfo->WriterCloseFormat, CP_WriterCloseHandler, NULL);
    CPInfo->ReaderCloseFormat =
        CMregister_simple_format(CPInfo->cm, "ReaderClose", ReaderCloseList,
                                 sizeof(struct _ReaderCloseMsg));
    CMregister_handler(CPInfo->ReaderCloseFormat, CP_ReaderCloseHandler, NULL);
}

static CP_GlobalInfo CPInfo = NULL;
static int CPInfoRefCount = 0;

extern void AddToLastCallFreeList(void *Block)
{
    CPInfo->LastCallFreeList =
        realloc(CPInfo->LastCallFreeList,
                sizeof(void *) * (CPInfo->LastCallFreeCount + 1));
    CPInfo->LastCallFreeList[CPInfo->LastCallFreeCount] = Block;
    CPInfo->LastCallFreeCount++;
}

extern void SstStreamDestroy(SstStream Stream)
{
    CP_verbose(Stream, "Destroying stream %p, name %s\n", Stream,
               Stream->Filename);
    if (Stream->Role == ReaderRole)
    {
        Stream->DP_Interface->destroyReader(&Svcs, Stream->DP_Stream);
    }
    else
    {
        Stream->DP_Interface->destroyWriter(&Svcs, Stream->DP_Stream);
    }
    if (Stream->Readers)
    {
        for (int i = 0; i < Stream->ReaderCount; i++)
        {
            CP_PeerConnection *connections_to_reader =
                Stream->Readers[i]->Connections;

            for (int j = 0; j < Stream->Readers[i]->ReaderCohortSize; j++)
            {
                free_attr_list(connections_to_reader[j].ContactList);
            }
            free(Stream->Readers[i]->Connections);
            free(Stream->Readers[i]->Peers);
            // Stream->Readers[i] is free'd in LastCall
        }
        free(Stream->Readers);
    }

    free(Stream->DataTransport);
    FFSFormatList FFSList = Stream->PreviousFormats;
    while (FFSList)
    {
        FFSFormatList Tmp = FFSList->Next;
        /* Server rep and ID here are copied */
        free(FFSList->FormatServerRep);
        free(FFSList->FormatIDRep);
        free(FFSList);
        FFSList = Tmp;
    }
    if ((Stream->Role == WriterRole) && Stream->WriterParams->FFSmarshal)
    {
        FFSFreeMarshalData(Stream);
        if (Stream->M)
            free(Stream->M);
        if (Stream->D)
            free(Stream->D);
    }

    if (Stream->Role == ReaderRole)
    {
        /* reader side */
        if (Stream->ReaderFFSContext)
            free_FFSContext(Stream->ReaderFFSContext);
        for (int i = 0; i < Stream->WriterCohortSize; i++)
        {
            free_attr_list(Stream->ConnectionsToWriter[i].ContactList);
            if (Stream->ConnectionsToWriter[i].CMconn)
            {
                CMConnection_close(Stream->ConnectionsToWriter[i].CMconn);
            }
        }
        if (Stream->ConnectionsToWriter)
            free(Stream->ConnectionsToWriter);
        free(Stream->Peers);
    }

    if (Stream->Filename)
        free(Stream->Filename);
    if (Stream->ParamsBlock)
        free(Stream->ParamsBlock);
    //   Stream is free'd in LastCall

    CPInfoRefCount--;
    if (CPInfoRefCount == 0)
    {
        CP_verbose(
            Stream,
            "Reference count now zero, Destroying process SST info cache\n");
        CManager_close(CPInfo->cm);
        CP_verbose(Stream, "After CManager_close\n");
        if (CPInfo->ffs_c)
            free_FFSContext(CPInfo->ffs_c);
        CP_verbose(Stream, "After freeing FFSContext\n");
        FreeCustomStructs(CPInfo);
        for (int i = 0; i < CPInfo->LastCallFreeCount; i++)
        {
            free(CPInfo->LastCallFreeList[i]);
        }
        free(CPInfo);
        CPInfo = NULL;
        if (CP_SstParamsList)
            free_FMfield_list(CP_SstParamsList);
        CP_SstParamsList = NULL;
    }
    CP_verbose(Stream, "Done with SstStreamDestroy()\n");
}

extern CP_GlobalInfo CP_getCPInfo(CP_DP_Interface DPInfo)
{

    if (CPInfo)
    {
        CPInfoRefCount++;
        return CPInfo;
    }

    initAtomList();

    CPInfo = malloc(sizeof(*CPInfo));
    memset(CPInfo, 0, sizeof(*CPInfo));

    CPInfo->cm = CManager_create();
    CMfork_comm_thread(CPInfo->cm);

    attr_list listen_list = create_attr_list();
    set_string_attr(listen_list, CM_TRANSPORT_ATOM, strdup("enet"));
    CMlisten_specific(CPInfo->cm, listen_list);
    free_attr_list(listen_list);

    CPInfo->fm_c = CMget_FMcontext(CPInfo->cm);
    CPInfo->ffs_c = create_FFSContext_FM(CPInfo->fm_c);

    if (!CP_SstParamsList)
    {
        int i = 0;
        /* need to pre-process the CP_SstParamsList to fix typedecl values */
        CP_SstParamsList = copy_field_list(CP_SstParamsList_RAW);
        while (CP_SstParamsList[i].field_name)
        {
            if ((strcmp(CP_SstParamsList[i].field_type, "int") == 0) ||
                (strcmp(CP_SstParamsList[i].field_type, "size_t") == 0))
            {
                free((void *)CP_SstParamsList[i].field_type);
                CP_SstParamsList[i].field_type = strdup("integer");
            }
            else if ((strcmp(CP_SstParamsList[i].field_type, "char*") == 0) ||
                     (strcmp(CP_SstParamsList[i].field_type, "char *") == 0))
            {
                free((void *)CP_SstParamsList[i].field_type);
                CP_SstParamsList[i].field_type = strdup("string");
            }
            i++;
        }
    }
    for (int i = 0; i < sizeof(CP_DP_WriterArrayStructs) /
                            sizeof(CP_DP_WriterArrayStructs[0]);
         i++)
    {
        if (CP_DP_WriterArrayStructs[i].format_name &&
            (strcmp(CP_DP_WriterArrayStructs[i].format_name, "SstParams") == 0))
        {
            CP_DP_WriterArrayStructs[i].field_list = CP_SstParamsList;
        }
    }

    for (int i = 0; i < sizeof(CP_WriterResponseStructs) /
                            sizeof(CP_WriterResponseStructs[0]);
         i++)
    {
        if (CP_WriterResponseStructs[i].format_name &&
            (strcmp(CP_WriterResponseStructs[i].format_name, "SstParams") == 0))
        {
            CP_WriterResponseStructs[i].field_list = CP_SstParamsList;
        }
    }

    doFormatRegistration(CPInfo, DPInfo);

    CPInfoRefCount++;
    return CPInfo;
}

SstStream CP_newStream()
{
    SstStream Stream = malloc(sizeof(*Stream));
    memset(Stream, 0, sizeof(*Stream));
    pthread_mutex_init(&Stream->DataLock, NULL);
    pthread_cond_init(&Stream->DataCondition, NULL);
    Stream->WriterTimestep = -1; // Filled in by ProvideTimestep
    Stream->ReaderTimestep = -1; // first beginstep will get us timestep 0
    if (getenv("SstVerbose"))
    {
        Stream->Verbose = 1;
    }
    else
    {
        Stream->Verbose = 0;
    }
    return Stream;
}

static void DP_verbose(SstStream Stream, char *Format, ...);
static CManager CP_getCManager(SstStream Stream);
static void CP_sendToPeer(SstStream Stream, CP_PeerCohort cohort, int rank,
                          CMFormat Format, void *data);
static MPI_Comm CP_getMPIComm(SstStream Stream);

struct _CP_Services Svcs = {
    (CP_VerboseFunc)DP_verbose, (CP_GetCManagerFunc)CP_getCManager,
    (CP_SendToPeerFunc)CP_sendToPeer, (CP_GetMPICommFunc)CP_getMPIComm};

extern int *setupPeerArray(int MySize, int MyRank, int PeerSize)
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
