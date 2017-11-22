#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <atl.h>
#include <evpath.h>
#include <mpi.h>

#include "sst.h"

#include "cp_internal.h"

void CP_parseParams(SstStream Stream, const char *Params)
{
    Stream->WaitForFirstReader = 1;
}

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

static FMField SstMetadataList[] = {{"DataSize", "integer", sizeof(size_t),
                                     FMOffset(struct _SstMetadata *, DataSize)},
                                    {"VarCount", "integer", sizeof(int),
                                     FMOffset(struct _SstMetadata *, VarCount)},
                                    {"Vars", "VarMetadata[VarCount]",
                                     sizeof(struct _SstVarMeta),
                                     FMOffset(struct _SstMetadata *, Vars)},
                                    {NULL, NULL, 0, 0}};

static FMField SstVarMetaList[] = {
    {"VarName", "string", sizeof(char *),
     FMOffset(struct _SstVarMeta *, VarName)},
    {"DimensionCount", "integer", sizeof(int),
     FMOffset(struct _SstVarMeta *, DimensionCount)},
    {"Dimensions", "VarDimension[DimensionCount]", sizeof(struct _SstDimenMeta),
     FMOffset(struct _SstVarMeta *, Dimensions)},
    {"DataOffsetInBlock", "integer", sizeof(int),
     FMOffset(struct _SstVarMeta *, DataOffsetInBlock)},
    {NULL, NULL, 0, 0}};

static FMField SstDimenMetaList[] = {
    {"Offset", "integer", sizeof(int),
     FMOffset(struct _SstDimenMeta *, Offset)},
    {"Size", "integer", sizeof(int), FMOffset(struct _SstDimenMeta *, Size)},
    {"GlobalSize", "integer", sizeof(int),
     FMOffset(struct _SstDimenMeta *, GlobalSize)},
    {NULL, NULL, 0, 0}};

static FMField MetaDataPlusDPInfoList[] = {
    {"Metadata", "*SstMetadata", sizeof(struct _SstMetadata),
     FMOffset(struct _MetadataPlusDPInfo *, Metadata)},
    {"DP_TimestepInfo", "*DP_STRUCT", 0,
     FMOffset(struct _MetadataPlusDPInfo *, DP_TimestepInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec MetaDataPlusDPInfoStructs[] = {
    {"MetaDataPlusDPInfo", MetaDataPlusDPInfoList,
     sizeof(struct _MetadataPlusDPInfo), NULL},
    {"SstMetadata", SstMetadataList, sizeof(struct _SstMetadata), NULL},
    {"VarMetadata", SstVarMetaList, sizeof(struct _SstVarMeta), NULL},
    {"VarDimension", SstDimenMetaList, sizeof(struct _SstDimenMeta), NULL},
    {NULL, NULL, 0, NULL}};

static FMField TimestepMetadataList[] = {
    {"RS_Stream", "integer", sizeof(void *),
     FMOffset(struct _TimestepMetadataMsg *, RS_Stream)},
    {"timestep", "integer", sizeof(int),
     FMOffset(struct _TimestepMetadataMsg *, Timestep)},
    {"cohort_size", "integer", sizeof(int),
     FMOffset(struct _TimestepMetadataMsg *, CohortSize)},
    {"metadata", "(*SstMetadata)[cohort_size]", sizeof(struct _SstMetadata),
     FMOffset(struct _TimestepMetadataMsg *, Metadata)},
    {"TP_TimestepInfo", "(*DP_STRUCT)[cohort_size]", 0,
     FMOffset(struct _TimestepMetadataMsg *, DP_TimestepInfo)},
    {NULL, NULL, 0, 0}};

static FMStructDescRec TimestepMetadataStructs[] = {
    {"timestepMetadata", TimestepMetadataList,
     sizeof(struct _TimestepMetadataMsg), NULL},
    {"SstMetadata", SstMetadataList, sizeof(struct _SstMetadata), NULL},
    {"VarMetadata", SstVarMetaList, sizeof(struct _SstVarMeta), NULL},
    {"VarDimension", SstDimenMetaList, sizeof(struct _SstDimenMeta), NULL},
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
    FMStructDescList CombinedFormats = NULL;
    int i = 0, topCount = 0, cpCount = 0, dpCount = 0;
    CombinedFormats = FMcopy_struct_list(top);

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
               MPI_COMM_WORLD);

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
                MPI_CHAR, 0, MPI_COMM_WORLD);

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
        MPI_Bcast(&DataSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(tmp, DataSize, MPI_CHAR, 0, MPI_COMM_WORLD);
        Buffer = malloc(DataSize);
        memcpy(Buffer, tmp, DataSize);
        free_FFSBuffer(Buf);
    }
    else
    {
        MPI_Bcast(&DataSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
        Buffer = malloc(DataSize);
        MPI_Bcast(Buffer, DataSize, MPI_CHAR, 0, MPI_COMM_WORLD);
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
                  MPI_COMM_WORLD);

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
                   MPI_CHAR, MPI_COMM_WORLD);

    FFSContext context = Stream->CPInfo->ffs_c;

    Pointers = malloc(Stream->CohortSize * sizeof(Pointers[0]));
    for (i = 0; i < Stream->CohortSize; i++)
    {
        FFSdecode_in_place(context, RecvBuffer + Displs[i],
                           (void **)&Pointers[i]);
        //    FFSTypeHandle ffs_type = FFSTypeHandle_from_encode(context,
        //    RecvBuffer);
        //        printf("Decode for rank %d :\n", i);
        //        FMdump_data(FMFormat_of_original(ffs_type), Pointers[i],
        //        1024000);
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

    FullReaderRegisterStructs =
        combineCpDpFormats(CP_ReaderRegisterStructs, CP_ReaderInitStructs,
                           DPInfo->ReaderContactFormats);
    CPInfo->ReaderRegisterFormat =
        CMregister_format(CPInfo->cm, FullReaderRegisterStructs);
    CMregister_handler(CPInfo->ReaderRegisterFormat, CP_ReaderRegisterHandler,
                       NULL);

    CombinedReaderStructs =
        combineCpDpFormats(CP_DP_ReaderArrayStructs, CP_ReaderInitStructs,
                           DPInfo->ReaderContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, CombinedReaderStructs);
    CPInfo->CombinedReaderInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, CombinedReaderStructs);

    PerRankWriterStructs =
        combineCpDpFormats(CP_DP_WriterPairStructs, CP_WriterInitStructs,
                           DPInfo->WriterContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, PerRankWriterStructs);
    CPInfo->PerRankWriterInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, PerRankWriterStructs);

    FullWriterResponseStructs =
        combineCpDpFormats(CP_WriterResponseStructs, CP_WriterInitStructs,
                           DPInfo->WriterContactFormats);
    CPInfo->WriterResponseFormat =
        CMregister_format(CPInfo->cm, FullWriterResponseStructs);
    CMregister_handler(CPInfo->WriterResponseFormat, CP_WriterResponseHandler,
                       NULL);

    CombinedWriterStructs =
        combineCpDpFormats(CP_DP_WriterArrayStructs, CP_WriterInitStructs,
                           DPInfo->WriterContactFormats);
    f = FMregister_data_format(CPInfo->fm_c, CombinedWriterStructs);
    CPInfo->CombinedWriterInfoFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, CombinedWriterStructs);

    CombinedMetadataStructs = combineCpDpFormats(
        MetaDataPlusDPInfoStructs, NULL, DPInfo->TimestepInfoFormats);
    f = FMregister_data_format(CPInfo->fm_c, CombinedMetadataStructs);
    CPInfo->PerRankMetadataFormat =
        FFSTypeHandle_by_index(CPInfo->ffs_c, FMformat_index(f));
    FFSset_fixed_target(CPInfo->ffs_c, CombinedMetadataStructs);

    CombinedTimestepMetadataStructs = combineCpDpFormats(
        TimestepMetadataStructs, NULL, DPInfo->TimestepInfoFormats);
    CPInfo->DeliverTimestepMetadataFormat =
        CMregister_format(CPInfo->cm, CombinedTimestepMetadataStructs);
    CMregister_handler(CPInfo->DeliverTimestepMetadataFormat,
                       CP_TimestepMetadataHandler, NULL);

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
}

extern CP_GlobalInfo CP_getCPInfo(CP_DP_Interface DPInfo)
{
    static CP_GlobalInfo CPInfo = NULL;

    if (CPInfo)
        return CPInfo;

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

    doFormatRegistration(CPInfo, DPInfo);

    return CPInfo;
}

SstStream CP_newStream()
{
    SstStream Stream = malloc(sizeof(*Stream));
    memset(Stream, 0, sizeof(*Stream));
    pthread_mutex_init(&Stream->DataLock, NULL);
    pthread_cond_init(&Stream->DataCondition, NULL);
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
