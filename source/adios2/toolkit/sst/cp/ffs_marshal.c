#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <atl.h>
#include <evpath.h>
#include <mpi.h>
#include <pthread.h>

#include "sst.h"

#include "cp_internal.h"

typedef struct _FFSWriterRec
{
    void *Key;
    int FieldID;
    size_t DataOffset;
    size_t MetaOffset;
    int SingleValue;
} * FFSWriterRec;

struct FFSWriterMarshalBase
{
    int RecCount;
    FFSWriterRec RecList;
    FMContext LocalFMContext;
    int MetaFieldCount;
    FMFieldList MetaFields;
    FMFormat MetaFormat;
    int DataFieldCount;
    FMFieldList DataFields;
    FMFormat DataFormat;
};

typedef struct FFSVarRec
{
    void *Variable;
    char *VarName;
    FMFieldList *PerWriterMetaFieldDesc;
    FMFieldList *PerWriterDataFieldDesc;
    size_t DimCount;
    size_t *GlobalDims;
    size_t **PerWriterStart;
    size_t **PerWriterCounts;
    void **PerWriterIncomingData;
} * FFSVarRec;

typedef struct FFSArrayRequest
{
    FFSVarRec VarRec;
    size_t *Start;
    size_t *Count;
    void *Data;
    struct FFSArrayRequest *Next;
} * FFSArrayRequest;

enum WriterDataStatusEnum
{
    Empty = 0,
    Needed = 1,
    Requested = 2,
    Full = 3
};

typedef struct FFSReaderPerWriterRec
{
    enum WriterDataStatusEnum Status;
    char *RawBuffer;
    DP_CompletionHandle ReadHandle;
} FFSReaderPerWriterRec;

struct FFSReaderMarshalBase
{
    int VarCount;
    FFSVarRec VarList;
    FMContext LocalFMContext;
    FFSArrayRequest PendingVarRequests;

    void **MetadataBaseAddrs;
    FMFieldList *MetadataFieldLists;

    void **DataBaseAddrs;
    FMFieldList *DataFieldLists;

    FFSReaderPerWriterRec *WriterInfo;
};

static char *ConcatName(const char *base_name, const char *postfix)
{
    char *Ret =
        malloc(strlen("SST_") + strlen(base_name) + strlen(postfix) + 1);
    strcpy(Ret, "SST_");
    strcat(Ret, base_name);
    strcat(Ret, postfix);
    return Ret;
}

static char *BuildArrayName(const char *base_name, const char *type)
{
    int Len = strlen(base_name) + strlen(type) + strlen("SST_") + 16;
    char *Ret = malloc(Len);
    sprintf(Ret, "SST%d_", (int)strlen(type));
    strcat(Ret, type);
    strcat(Ret, "_");
    strcat(Ret, base_name);
    strcat(Ret, "Dims");
    return Ret;
}

static void BreakdownArrayName(const char *Name, char **base_name_p,
                               char **type_p)
{
    int TypeLen;
    char *TypeStart = index(Name, '_') + 1;
    const char *NameStart;
    sscanf(Name, "SST%d_", &TypeLen);
    NameStart = Name + TypeLen + 6;
    *type_p = malloc(TypeLen + 1);
    strncpy(*type_p, TypeStart, TypeLen);
    (*type_p)[TypeLen] = 0;
    *base_name_p = strdup(NameStart);
    (*base_name_p)[strlen(*base_name_p) - 4] = 0; // kill "Dims"
}

static char *TranslateADIOS2Type2FFS(const char *Type)
{
    if (strcmp(Type, "char") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "signed char") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "unsigned char") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "short") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "unsigned short") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "int") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "unsigned int") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "long int") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "long long int") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "unsigned long int") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "unsigned long long int") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "float") == 0)
    {
        return strdup("float");
    }
    else if (strcmp(Type, "double") == 0)
    {
        return strdup("float");
    }
    else if (strcmp(Type, "long double") == 0)
    {
        return strdup("float");
    }
    return strdup(Type);
}

static char *TranslateFFSType2ADIOS(const char *Type, int size)
{
    if (strcmp(Type, "integer") == 0)
    {
        if (size == sizeof(int))
        {
            return strdup("int");
        }
        else if (size == sizeof(long))
        {
            return strdup("long int");
        }
        else if (size == sizeof(char))
        {
            return strdup("char");
        }
        else if (size == sizeof(short))
        {
            return strdup("short");
        }
    }
    else if (strcmp(Type, "unsigned integer") == 0)
    {
        if (size == sizeof(int))
        {
            return strdup("unsigned int");
        }
        else if (size == sizeof(long))
        {
            return strdup("unsigned long int");
        }
        else if (size == sizeof(char))
        {
            return strdup("unsigned char");
        }
        else if (size == sizeof(short))
        {
            return strdup("unsigned short");
        }
    }
    else if (strcmp(Type, "double") == 0)
    {
        if (size == sizeof(float))
        {
            return strdup("float");
        }
        else if (size == sizeof(long double))
        {
            return strdup("long double");
        }
        else
        {
            return strdup("double");
        }
    }
    return strdup(Type);
}

static void RecalcMarshalStorageSize(SstStream Stream)
{
    struct FFSWriterMarshalBase *Info = Stream->MarshalData;
    if (Info->DataFieldCount)
    {
        FMFieldList LastDataField;
        size_t NewDataSize;
        LastDataField = &Info->DataFields[Info->DataFieldCount - 1];
        NewDataSize =
            (LastDataField->field_offset + LastDataField->field_size + 7) & ~7;
        Stream->D = realloc(Stream->D, NewDataSize + 8);
        memset(Stream->D + Stream->DataSize, 0, NewDataSize - Stream->DataSize);
        Stream->DataSize = NewDataSize;
    }
    if (Info->MetaFieldCount)
    {
        FMFieldList LastMetaField;
        size_t NewMetaSize;
        LastMetaField = &Info->MetaFields[Info->MetaFieldCount - 1];
        NewMetaSize =
            (LastMetaField->field_offset + LastMetaField->field_size + 7) & ~7;
        Stream->M = realloc(Stream->M, NewMetaSize + 8);
        memset(Stream->M + Stream->MetadataSize, 0,
               NewMetaSize - Stream->MetadataSize);
        Stream->MetadataSize = NewMetaSize;
    }
}

static void AddSimpleField(FMFieldList *FieldP, int *CountP, const char *Name,
                           const char *Type, int ElementSize)
{
    int FieldNum = *CountP, Offset = 0;
    FMFieldList Field;
    if (*CountP)
    {
        FMFieldList PriorField;
        PriorField = &((*FieldP)[(*CountP) - 1]);
        Offset = ((PriorField->field_offset + PriorField->field_size +
                   ElementSize - 1) /
                  ElementSize) *
                 ElementSize;
    }
    *FieldP = realloc(*FieldP, (*CountP + 2) * sizeof((*FieldP)[0]));
    Field = &((*FieldP)[*CountP]);
    (*CountP)++;
    Field->field_name = Name;
    Field->field_type = Type;
    Field->field_size = ElementSize;
    Field->field_offset = Offset;
    Field++;
    Field->field_name = NULL;
    Field->field_type = NULL;
    Field->field_size = 0;
    Field->field_offset = 0;
}

static void AddField(FMFieldList *FieldP, int *CountP, const char *Name,
                     const char *Type, int ElementSize)
{
    AddSimpleField(FieldP, CountP, Name, TranslateADIOS2Type2FFS(Type),
                   ElementSize);
}

static void AddFixedArrayField(FMFieldList *FieldP, int *CountP,
                               const char *Name, const char *Type,
                               int ElementSize, int DimCount)
{
    const char *TransType = TranslateADIOS2Type2FFS(Type);
    char *TypeWithArray = malloc(strlen(TransType) + 16);
    sprintf(TypeWithArray, "*(%s[%d])", TransType, DimCount);
    free((void *)TransType);
    AddSimpleField(FieldP, CountP, Name, TypeWithArray, sizeof(void *));
    (*FieldP)[*CountP - 1].field_size = ElementSize;
}
static void AddVarArrayField(FMFieldList *FieldP, int *CountP, const char *Name,
                             const char *Type, int ElementSize, char *SizeField)
{
    char *TransType = TranslateADIOS2Type2FFS(Type);
    char *TypeWithArray = malloc(strlen(TransType) + strlen(SizeField) + 8);
    sprintf(TypeWithArray, "%s[%s]", TransType, SizeField);
    free(TransType);
    AddSimpleField(FieldP, CountP, Name, TypeWithArray, sizeof(void *));
    (*FieldP)[*CountP - 1].field_size = ElementSize;
}

struct FFSMetadataInfoStruct
{
    size_t BitFieldCount;
    size_t *BitField;
    size_t DataBlockSize;
};

static void InitMarshalData(SstStream Stream)
{
    struct FFSWriterMarshalBase *Info =
        malloc(sizeof(struct FFSWriterMarshalBase));
    struct FFSMetadataInfoStruct *MBase;
    FMFieldList Field;

    Stream->MarshalData = Info;
    Info->RecCount = 0;
    Info->RecList = malloc(sizeof(Info->RecList[0]));
    Info->MetaFieldCount = 0;
    Info->MetaFields = malloc(sizeof(Info->MetaFields[0]));
    Info->DataFieldCount = 0;
    Info->DataFields = malloc(sizeof(Info->DataFields[0]));
    Info->LocalFMContext = create_local_FMcontext();
    AddSimpleField(&Info->MetaFields, &Info->MetaFieldCount,
                   strdup("BitFieldCount"), strdup("integer"), sizeof(size_t));
    AddSimpleField(&Info->MetaFields, &Info->MetaFieldCount, strdup("BitField"),
                   strdup("integer[BitFieldCount]"), sizeof(size_t));
    AddSimpleField(&Info->MetaFields, &Info->MetaFieldCount,
                   strdup("DataBlockSize"), strdup("integer"), sizeof(size_t));
    RecalcMarshalStorageSize(Stream);
    MBase = Stream->M;
    MBase->BitFieldCount = 0;
    MBase->BitField = malloc(sizeof(size_t));
    MBase->DataBlockSize = 0;
}

static FFSWriterRec CreateWriterRec(SstStream Stream, void *Variable,
                                    const char *Name, const char *Type,
                                    size_t ElemSize, size_t DimCount)
{
    if (!Stream->MarshalData)
    {
        InitMarshalData(Stream);
    }
    struct FFSWriterMarshalBase *Info =
        (struct FFSWriterMarshalBase *)Stream->MarshalData;
    Info->RecList =
        realloc(Info->RecList, (Info->RecCount + 1) * sizeof(Info->RecList[0]));
    FFSWriterRec Rec = &Info->RecList[Info->RecCount];
    Rec->Key = Variable;
    Rec->FieldID = Info->RecCount;
    if (DimCount == 0)
    {
        // simple field, only add base value FMField to metadata
        AddField(&Info->MetaFields, &Info->MetaFieldCount, ConcatName(Name, ""),
                 Type, ElemSize);
        RecalcMarshalStorageSize(Stream);
        Rec->MetaOffset =
            Info->MetaFields[Info->MetaFieldCount - 1].field_offset;
        Rec->SingleValue = 1;
        Rec->DataOffset = (size_t)-1;
        // Changing the formats renders these invalid
        Info->MetaFormat = NULL;
    }
    else
    {
        // Array field.  To Metadata, add FMFields for DimCount, Shape, Count
        // and Offsets matching _MetaArrayRec
        AddField(&Info->MetaFields, &Info->MetaFieldCount,
                 BuildArrayName(Name, Type), "integer", sizeof(size_t));
        Rec->MetaOffset =
            Info->MetaFields[Info->MetaFieldCount - 1].field_offset;
        Rec->SingleValue = 0;
        RecalcMarshalStorageSize(Stream);
        AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount,
                           ConcatName(Name, "Shape"), "integer", sizeof(size_t),
                           DimCount);
        AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount,
                           ConcatName(Name, "Count"), "integer", sizeof(size_t),
                           DimCount);
        AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount,
                           ConcatName(Name, "Offsets"), "integer",
                           sizeof(size_t), DimCount);
        // To Data, add FMFields for ElemCount and Array matching _ArrayRec
        AddField(&Info->DataFields, &Info->DataFieldCount,
                 ConcatName(Name, "ElemCount"), "integer", sizeof(size_t));
        RecalcMarshalStorageSize(Stream);
        Rec->DataOffset =
            Info->DataFields[Info->DataFieldCount - 1].field_offset;
        AddVarArrayField(&Info->DataFields, &Info->DataFieldCount,
                         ConcatName(Name, ""), Type, ElemSize,
                         ConcatName(Name, "ElemCount"));
        // Changing the formats renders these invalid
        Info->MetaFormat = NULL;
        Info->DataFormat = NULL;
    }
    Info->RecCount++;
    return Rec;
}

typedef struct _ArrayRec
{
    size_t ElemCount;
    void *Array;
} ArrayRec;

typedef struct _MetaArrayRec
{
    size_t Dims;
    size_t *Shape;
    size_t *Count;
    size_t *Offsets;
} MetaArrayRec;

typedef struct _FFSTimestepInfo
{
    FFSBuffer MetaEncodeBuffer;
    FFSBuffer DataEncodeBuffer;
} * FFSTimestepInfo;

static void FreeTSInfo(void *ClientData)
{
    FFSTimestepInfo TSInfo = (FFSTimestepInfo)ClientData;
    free_FFSBuffer(TSInfo->MetaEncodeBuffer);
    free_FFSBuffer(TSInfo->DataEncodeBuffer);
    free(TSInfo);
}

static size_t *CopyDims(const size_t Count, const size_t *Vals)
{
    size_t *Ret = malloc(Count * sizeof(Ret[0]));
    memcpy(Ret, Vals, Count * sizeof(Ret[0]));
    return Ret;
}

static size_t CalcSize(const size_t Count, const size_t *Vals)
{
    size_t i;
    size_t Elems = 1;
    for (i = 0; i < Count; i++)
    {
        Elems *= Vals[i];
    }
    return Elems;
}

static FFSWriterRec LookupWriterRec(SstStream Stream, void *Key)
{
    struct FFSWriterMarshalBase *Info = Stream->MarshalData;

    if (!Stream->MarshalData)
        return NULL;

    for (int i = 0; i < Info->RecCount; i++)
    {
        if (Info->RecList[i].Key == Key)
        {
            return &Info->RecList[i];
        }
    }

    return NULL;
}

static FFSVarRec LookupVarByKey(SstStream Stream, void *Key)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    for (int i = 0; i < Info->VarCount; i++)
    {
        if (Info->VarList[i].Variable == Key)
        {
            return &Info->VarList[i];
        }
    }

    return NULL;
}

static FFSVarRec LookupVarByName(SstStream Stream, const char *Name)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    for (int i = 0; i < Info->VarCount; i++)
    {
        if (strcmp(Info->VarList[i].VarName, Name) == 0)
        {
            return &Info->VarList[i];
        }
    }

    return NULL;
}

static FFSVarRec CreateVarRec(SstStream Stream, const char *ArrayName)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;
    Info->VarList =
        realloc(Info->VarList, sizeof(Info->VarList[0]) * (Info->VarCount + 1));
    Info->VarList[Info->VarCount].VarName = strdup(ArrayName);
    Info->VarList[Info->VarCount].PerWriterMetaFieldDesc =
        calloc(sizeof(FMFieldList), Stream->WriterCohortSize);
    Info->VarList[Info->VarCount].PerWriterDataFieldDesc =
        calloc(sizeof(FMFieldList), Stream->WriterCohortSize);
    Info->VarList[Info->VarCount].PerWriterStart =
        calloc(sizeof(size_t *), Stream->WriterCohortSize);
    Info->VarList[Info->VarCount].PerWriterCounts =
        calloc(sizeof(size_t *), Stream->WriterCohortSize);
    Info->VarList[Info->VarCount].PerWriterIncomingData =
        calloc(sizeof(void *), Stream->WriterCohortSize);
    return &Info->VarList[Info->VarCount++];
}

// GSE - should be exported from FFS/FM, but isn't
extern int struct_size_field_list(FMFieldList list, int pointer_size);
extern void CP_verbose(SstStream Stream, char *Format, ...);

extern int SstWriterBeginStep(SstStream Stream, int mode,
                              const float timeout_sec)
{
    Stream->WriterTimestep++;
    return 0;
}

void SstReaderInitCallback(SstStream Stream, void *Reader,
                           VarSetupUpcallFunc VarCallback,
                           ArraySetupUpcallFunc ArrayCallback)
{
    Stream->VarSetupUpcall = VarCallback;
    Stream->ArraySetupUpcall = ArrayCallback;
    Stream->SetupUpcallReader = Reader;
}

extern void SstGetDeferred(SstStream Stream, void *Variable, const char *Name,
                           size_t DimCount, const unsigned long *Start,
                           const unsigned long *Count, void *Data)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;
    int GetFromWriter = 0;
    FFSVarRec Var = LookupVarByKey(Stream, Variable);

    // if Variable is in Metadata (I.E. DimCount == 0), move incoming data to
    // Data area
    if (DimCount == 0)
    {
        void *IncomingDataBase =
            ((char *)Info->MetadataBaseAddrs[GetFromWriter]) +
            Var->PerWriterMetaFieldDesc[GetFromWriter]->field_offset;
        memcpy(Data, IncomingDataBase,
               Var->PerWriterMetaFieldDesc[GetFromWriter]->field_size);
    }
    else
    {
        // Build request structure and enter it into requests list
        FFSArrayRequest Req = malloc(sizeof(*Req));
        Req->VarRec = Var;
        // make a copy of Start and Count request
        Req->Start = malloc(sizeof(Start[0]) * Var->DimCount);
        memcpy(Req->Start, Start, sizeof(Start[0]) * Var->DimCount);
        Req->Count = malloc(sizeof(Count[0]) * Var->DimCount);
        memcpy(Req->Count, Count, sizeof(Count[0]) * Var->DimCount);
        Req->Data = Data;
        Req->Next = Info->PendingVarRequests;
        Info->PendingVarRequests = Req;
    }
}

static int NeedWriter(FFSArrayRequest Req, int i)
{
    for (int j = 0; j < Req->VarRec->DimCount; j++)
    {
        size_t SelOffset = Req->Start[j];
        size_t SelSize = Req->Count[j];
        size_t RankOffset = Req->VarRec->PerWriterStart[i][j];
        size_t RankSize = Req->VarRec->PerWriterCounts[i][j];
        if ((SelSize == 0) || (RankSize == 0))
        {
            return 0;
        }
        if ((RankOffset < SelOffset && (RankOffset + RankSize) <= SelOffset) ||
            (RankOffset >= SelOffset + SelSize))
        {
            return 0;
        }
    }
    return 1;
}

static void IssueReadRequests(SstStream Stream, FFSArrayRequest Reqs)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    while (Reqs)
    {
        for (int i = 0; i < Stream->WriterCohortSize; i++)
        {
            if (NeedWriter(Reqs, i))
            {
                Info->WriterInfo[i].Status = Needed;
            }
        }
        Reqs = Reqs->Next;
    }

    for (int i = 0; i < Stream->WriterCohortSize; i++)
    {
        if (Info->WriterInfo[i].Status == Needed)
        {
            size_t DataSize =
                ((struct FFSMetadataInfoStruct *)Info->MetadataBaseAddrs[i])
                    ->DataBlockSize;
            Info->WriterInfo[i].RawBuffer =
                realloc(Info->WriterInfo[i].RawBuffer, DataSize);
            Info->WriterInfo[i].ReadHandle = SstReadRemoteMemory(
                Stream, i, Stream->ReaderTimestep, 0, DataSize,
                Info->WriterInfo[i].RawBuffer, NULL);
            Info->WriterInfo[i].Status = Requested;
        }
    }
}

static void ClearReadRequests(SstStream Stream)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    FFSArrayRequest Req = Info->PendingVarRequests;

    while (Req)
    {
        free(Req);
        Req = Req->Next;
    }
    Info->PendingVarRequests = NULL;
}

static void DecodeAndPrepareData(SstStream Stream, int Writer)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    FFSReaderPerWriterRec *WriterInfo = &Info->WriterInfo[Writer];
    FFSTypeHandle FFSformat;
    FMFieldList FieldList;
    FMStructDescList FormatList;
    void *BaseData;

    FFSformat = FFSTypeHandle_from_encode(Stream->ReaderFFSContext,
                                          WriterInfo->RawBuffer);

    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(Stream->ReaderFFSContext);
        FMFormat Format = FMformat_from_ID(FMC, WriterInfo->RawBuffer);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        FMlocalize_structs(List);
        establish_conversion(Stream->ReaderFFSContext, FFSformat, List);
    }
    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(Stream->ReaderFFSContext, WriterInfo->RawBuffer,
                           &BaseData);
    }
    else
    {
        size_t DataSize =
            ((struct FFSMetadataInfoStruct *)Info->MetadataBaseAddrs[Writer])
                ->DataBlockSize;
        int DecodedLength = FFS_est_decode_length(
            Stream->ReaderFFSContext, WriterInfo->RawBuffer, DataSize);
        BaseData = malloc(DecodedLength);
        FFSBuffer decode_buf = create_fixed_FFSBuffer(BaseData, DecodedLength);
        FFSdecode_to_buffer(Stream->ReaderFFSContext, WriterInfo->RawBuffer,
                            decode_buf);
    }
    //    printf("\nIncomingDatablock is %p :\n", BaseData);
    //    FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
    Info->DataBaseAddrs[Writer] = BaseData;
    FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
    FieldList = FormatList[0].field_list;
    Info->DataFieldLists[Writer] = FieldList;

    int i = 0;
    while (FieldList[i].field_name)
    {
        ArrayRec *data_base = BaseData + FieldList[i].field_offset;
        const char *ArrayName = FieldList[i + 1].field_name + 4;
        FFSVarRec VarRec = LookupVarByName(Stream, ArrayName);
        VarRec->PerWriterIncomingData[Writer] = data_base->Array;
        VarRec->PerWriterDataFieldDesc[Writer] = &FieldList[i + 1];
        i += 2;
    }
}

static void WaitForReadRequests(SstStream Stream)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    for (int i = 0; i < Stream->WriterCohortSize; i++)
    {
        if (Info->WriterInfo[i].Status == Requested)
        {
            SstStatusValue Result =
                SstWaitForCompletion(Stream, Info->WriterInfo[i].ReadHandle);
            if (Result == SstSuccess)
            {
                Info->WriterInfo[i].Status = Full;
                DecodeAndPrepareData(Stream, i);
            }
            else
            {
                /* handle errors here */
            }
        }
    }
}

static void MapLocalToGlobalIndex(size_t Dims, const size_t *LocalIndex,
                                  const size_t *LocalOffsets,
                                  size_t *GlobalIndex)
{
    for (int i = 0; i < Dims; i++)
    {
        GlobalIndex[i] = LocalIndex[i] + LocalOffsets[i];
    }
}

static void MapGlobalToLocalIndex(size_t Dims, const size_t *GlobalIndex,
                                  const size_t *LocalOffsets,
                                  size_t *LocalIndex)
{
    for (int i = 0; i < Dims; i++)
    {
        LocalIndex[i] = GlobalIndex[i] - LocalOffsets[i];
    }
}

static int FindOffset(size_t Dims, const size_t *Size, const size_t *Index)
{
    int Offset = 0;
    for (int i = 0; i < Dims; i++)
    {
        Offset = Index[i] + (Size[i] * Offset);
    }
    return Offset;
}

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/*
 *  - ElementSize is the byte size of the array elements
 *  - Dims is the number of dimensions in the variable
 *  - GlobalDims is an array, Dims long, giving the size of each dimension
 *  - PartialOffsets is an array, Dims long, giving the starting offsets per
 *    dimension of this data block in the global array
 *  - PartialCounts is an array, Dims long, giving the size per dimension
 *    of this data block in the global array
 *  - SelectionOffsets is an array, Dims long, giving the starting offsets in
 * the
 *    global array of the output selection.
 *  - SelectionCounts is an array, Dims long, giving the size per dimension
 *    of the output selection.
 *  - InData is the input, a slab of the global array
 *  - OutData is the output, to be filled with the selection array.
 */
void ExtractSelectionFromPartial(int ElementSize, size_t Dims,
                                 const size_t *GlobalDims,
                                 const size_t *PartialOffsets,
                                 const size_t *PartialCounts,
                                 const size_t *SelectionOffsets,
                                 const size_t *SelectionCounts,
                                 const char *InData, char *OutData)
{
    int BlockSize;
    int SourceBlockStride;
    int DestBlockStride;
    int SourceBlockStartOffset;
    int DestBlockStartOffset;
    int BlockCount;
    int OperantDims;
    int OperantElementSize;

    BlockSize = 1;
    OperantDims = Dims;
    OperantElementSize = ElementSize;
    for (int Dim = Dims - 1; Dim >= 0; Dim--)
    {
        if ((GlobalDims[Dim] == PartialCounts[Dim]) &&
            (SelectionCounts[Dim] == PartialCounts[Dim]))
        {
            BlockSize *= GlobalDims[Dim];
            OperantDims--; /* last dimension doesn't matter, we got all and we
                               want all */
            OperantElementSize *= GlobalDims[Dim];
        }
        else
        {
            int Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
            int Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                            SelectionOffsets[Dim] + SelectionCounts[Dim]);
            BlockSize *= (Right - Left);
            break;
        }
    }
    SourceBlockStride = PartialCounts[OperantDims - 1] * OperantElementSize;
    DestBlockStride = SelectionCounts[OperantDims - 1] * OperantElementSize;

    /* calculate first selected element and count */
    BlockCount = 1;
    size_t *FirstIndex = malloc(Dims * sizeof(FirstIndex[0]));
    for (int Dim = 0; Dim < Dims; Dim++)
    {
        int Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
        int Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                        SelectionOffsets[Dim] + SelectionCounts[Dim]);
        if (Dim < OperantDims - 1)
        {
            BlockCount *= (Right - Left);
        }
        FirstIndex[Dim] = Left;
    }
    size_t *SelectionIndex = malloc(Dims * sizeof(SelectionIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, SelectionOffsets, SelectionIndex);
    DestBlockStartOffset = FindOffset(Dims, SelectionCounts, SelectionIndex);
    free(SelectionIndex);
    DestBlockStartOffset *= ElementSize;

    size_t *PartialIndex = malloc(Dims * sizeof(PartialIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, PartialOffsets, PartialIndex);
    SourceBlockStartOffset = FindOffset(Dims, PartialCounts, PartialIndex);
    free(PartialIndex);
    SourceBlockStartOffset *= ElementSize;

    InData += SourceBlockStartOffset;
    OutData += DestBlockStartOffset;
    int i;
    for (i = 0; i < BlockCount; i++)
    {
        memcpy(OutData, InData, BlockSize * ElementSize);
        InData += SourceBlockStride;
        OutData += DestBlockStride;
    }
    free(FirstIndex);
}

static void FillReadRequests(SstStream Stream, FFSArrayRequest Reqs)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    while (Reqs)
    {
        for (int i = 0; i < Stream->WriterCohortSize; i++)
        {
            if (NeedWriter(Reqs, i))
            {
                /* if needed this writer fill destination with acquired data */
                int ElementSize =
                    Reqs->VarRec->PerWriterDataFieldDesc[i]->field_size;
                int DimCount = Reqs->VarRec->DimCount;
                size_t *GlobalDimensions = Reqs->VarRec->GlobalDims;
                size_t *RankOffset = Reqs->VarRec->PerWriterStart[i];
                size_t *RankSize = Reqs->VarRec->PerWriterCounts[i];
                size_t *SelOffset = Reqs->Start;
                size_t *SelSize = Reqs->Count;
                void *IncomingData = Reqs->VarRec->PerWriterIncomingData[i];

                ExtractSelectionFromPartial(
                    ElementSize, DimCount, GlobalDimensions, RankOffset,
                    RankSize, SelOffset, SelSize, IncomingData, Reqs->Data);
            }
        }
        Reqs = Reqs->Next;
    }
}

extern void SstPerformGets(SstStream Stream)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;

    IssueReadRequests(Stream, Info->PendingVarRequests);

    WaitForReadRequests(Stream);

    FillReadRequests(Stream, Info->PendingVarRequests);

    ClearReadRequests(Stream);
}

extern void SstWriterEndStep(SstStream Stream)
{
    struct FFSWriterMarshalBase *Info =
        (struct FFSWriterMarshalBase *)Stream->MarshalData;
    struct FFSFormatBlock *Formats = NULL;

    CP_verbose(Stream, "Calling SstWriterEndStep\n");
    // if field lists have changed, register formats with FFS local context, add
    // to format chain
    if (!Info->MetaFormat)
    {
        struct FFSFormatBlock *Block = malloc(sizeof(*Block));
        FMFormat Format = FMregister_simple_format(
            Info->LocalFMContext, "MetaData", Info->MetaFields,
            struct_size_field_list(Info->MetaFields, sizeof(char *)));
        Info->MetaFormat = Format;
        Block->FormatServerRep =
            get_server_rep_FMformat(Format, &Block->FormatServerRepLen);
        Block->FormatIDRep =
            get_server_ID_FMformat(Format, &Block->FormatIDRepLen);
        Block->Next = NULL;
        Formats = Block;
    }
    if (!Info->DataFormat)
    {
        struct FFSFormatBlock *Block = malloc(sizeof(*Block));
        FMFormat Format = FMregister_simple_format(
            Info->LocalFMContext, "Data", Info->DataFields,
            struct_size_field_list(Info->DataFields, sizeof(char *)));
        Info->DataFormat = Format;
        Block->FormatServerRep =
            get_server_rep_FMformat(Format, &Block->FormatServerRepLen);
        Block->FormatIDRep =
            get_server_ID_FMformat(Format, &Block->FormatIDRepLen);
        Block->Next = NULL;
        if (Formats)
        {
            Block->Next = Formats;
            Formats = Block;
        }
        else
        {
            Formats = Block;
        }
    }
    // Encode Metadata and Data to create contiguous data blocks
    FFSTimestepInfo TSInfo = malloc(sizeof(*TSInfo));
    FFSBuffer MetaEncodeBuffer = create_FFSBuffer();
    FFSBuffer DataEncodeBuffer = create_FFSBuffer();
    SstData DataRec = malloc(sizeof(*DataRec));
    SstData MetaDataRec = malloc(sizeof(*MetaDataRec));
    int MetaDataSize;
    int DataSize;
    struct FFSMetadataInfoStruct *MBase;
    DataRec->block =
        FFSencode(DataEncodeBuffer, Info->DataFormat, Stream->D, &DataSize);
    DataRec->DataSize = DataSize;
    TSInfo->DataEncodeBuffer = DataEncodeBuffer;

    MBase = Stream->M;
    MBase->DataBlockSize = DataSize;
    MetaDataRec->block =
        FFSencode(MetaEncodeBuffer, Info->MetaFormat, Stream->M, &MetaDataSize);
    MetaDataRec->DataSize = MetaDataSize;
    TSInfo->MetaEncodeBuffer = MetaEncodeBuffer;

    // Call SstInternalProvideStep with Metadata block, Data block and (any new)
    // formatID and formatBody
    //    printf("MetaDatablock is :\n");
    //    FMdump_encoded_data(Info->MetaFormat, MetaDataRec->block, 1024000);
    //    printf("\nDatablock is :\n");
    //    FMdump_encoded_data(Info->DataFormat, DataRec->block, 1024000);
    SstInternalProvideTimestep(Stream, MetaDataRec, DataRec,
                               Stream->WriterTimestep, Formats, FreeTSInfo,
                               TSInfo);
}

static void LoadFormats(SstStream Stream, FFSFormatList Formats)
{
    FFSFormatList Entry = Formats;
    while (Entry)
    {
        load_external_format_FMcontext(
            FMContext_from_FFS(Stream->ReaderFFSContext), Entry->FormatIDRep,
            Entry->FormatIDRepLen, Entry->FormatServerRep);
        Entry = Entry->Next;
    }
}

static int NameIndicatesArray(const char *Name)
{
    int Len = strlen(Name);
    return (strcmp("Dims", Name + Len - 4) == 0);
}

extern void FFSClearTimestepData(SstStream Stream)
{

    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;
    memset(Info->WriterInfo, 0,
           sizeof(Info->WriterInfo[0]) * Stream->WriterCohortSize);
    memset(Info->MetadataBaseAddrs, 0,
           sizeof(Info->MetadataBaseAddrs[0]) * Stream->WriterCohortSize);
    memset(Info->MetadataFieldLists, 0,
           sizeof(Info->MetadataFieldLists[0]) * Stream->WriterCohortSize);
    memset(Info->DataBaseAddrs, 0,
           sizeof(Info->DataBaseAddrs[0]) * Stream->WriterCohortSize);
    memset(Info->DataFieldLists, 0,
           sizeof(Info->DataFieldLists[0]) * Stream->WriterCohortSize);
    for (int i = 0; i < Info->VarCount; i++)
    {
        free(Info->VarList[i].VarName);
        free(Info->VarList[i].PerWriterMetaFieldDesc);
        free(Info->VarList[i].PerWriterDataFieldDesc);
        free(Info->VarList[i].PerWriterStart);
        free(Info->VarList[i].PerWriterCounts);
        free(Info->VarList[i].PerWriterIncomingData);
    }
    Info->VarCount = 0;
}

static void BuildVarList(SstStream Stream, TSMetadataMsg MetaData,
                         int WriterRank)
{
    FFSTypeHandle FFSformat;
    FMFieldList FieldList;
    FMStructDescList FormatList;
    void *BaseData;

    /* incoming metadata is all of our information about what was written
     * and is available to be read.  We'll process the data from each node
     * separately, but in such a way that we don't need to process the
     * MetaData again.  That means keeping around the information that we'll
     * need to respond to Get[Sync,Deferred] actions later.  For this we use
     * the VarList, which is keyed by the address of the Variable object
     * created at the ADIOS2 level.  So, we run through the individual
     * metadata blocks from a rank.  For each field set (one field for
     * atomic values, 4 for arrays), we look to see if we that name already
     * exists in the VarList (never for WriterRank==0), and if not we create
     * it and do the upcall to create the Variable object.  Then for each
     * Variable, we note global geometry, and for each rank we note the
     * FMField descriptor for its entry in the MetaData block and the
     * geometry for that block (Start + Offset arrays).  Also for each rank
     * we track the info that we'll need later (base address of decoded
     * metadata), format lists /buffers that might need freeing, etc.
     */

    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;
    if (!Info)
    {
        Info = malloc(sizeof(*Info));
        memset(Info, 0, sizeof(*Info));
        Stream->ReaderMarshalData = Info;
        Info->WriterInfo =
            calloc(sizeof(Info->WriterInfo[0]), Stream->WriterCohortSize);
        Info->MetadataBaseAddrs = calloc(sizeof(Info->MetadataBaseAddrs[0]),
                                         Stream->WriterCohortSize);
        Info->MetadataFieldLists = calloc(sizeof(Info->MetadataFieldLists[0]),
                                          Stream->WriterCohortSize);
        Info->DataBaseAddrs =
            calloc(sizeof(Info->DataBaseAddrs[0]), Stream->WriterCohortSize);
        Info->DataFieldLists =
            calloc(sizeof(Info->DataFieldLists[0]), Stream->WriterCohortSize);
    }

    FFSformat = FFSTypeHandle_from_encode(
        Stream->ReaderFFSContext, MetaData->Metadata[WriterRank]->BlockData);

    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(Stream->ReaderFFSContext);
        FMFormat Format =
            FMformat_from_ID(FMC, MetaData->Metadata[WriterRank]->BlockData);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        FMlocalize_structs(List);
        establish_conversion(Stream->ReaderFFSContext, FFSformat, List);
    }
    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(Stream->ReaderFFSContext,
                           MetaData->Metadata[WriterRank]->BlockData,
                           &BaseData);
    }
    else
    {
        int DecodedLength = FFS_est_decode_length(
            Stream->ReaderFFSContext, MetaData->Metadata[WriterRank]->BlockData,
            MetaData->Metadata[WriterRank]->BlockSize);
        BaseData = malloc(DecodedLength);
        FFSBuffer decode_buf = create_fixed_FFSBuffer(BaseData, DecodedLength);
        FFSdecode_to_buffer(Stream->ReaderFFSContext,
                            MetaData->Metadata[WriterRank]->BlockData,
                            decode_buf);
    }
    // printf("\nIncomingMetadatablock is %p :\n", BaseData);
    // FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
    Info->MetadataBaseAddrs[WriterRank] = BaseData;
    FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
    FieldList = FormatList[0].field_list;
    while (strncmp(FieldList->field_name, "BitField", 8) == 0)
        FieldList++;
    while (strncmp(FieldList->field_name, "DataBlockSize", 8) == 0)
        FieldList++;
    int i = 0;

    while (FieldList[i].field_name)
    {
        void *field_data = BaseData + FieldList[i].field_offset;
        if (NameIndicatesArray(FieldList[i].field_name))
        {
            MetaArrayRec *meta_base = field_data;
            char *ArrayName;
            char *Type;
            FFSVarRec VarRec = NULL;
            BreakdownArrayName(FieldList[i].field_name, &ArrayName, &Type);
            if (WriterRank != 0)
            {
                VarRec = LookupVarByName(Stream, ArrayName);
            }
            if (!VarRec)
            {
                VarRec = CreateVarRec(Stream, ArrayName);
                VarRec->DimCount = meta_base->Dims;
                VarRec->Variable = Stream->ArraySetupUpcall(
                    Stream->SetupUpcallReader, ArrayName, Type, meta_base->Dims,
                    meta_base->Shape, meta_base->Count, meta_base->Offsets);
            }
            if (WriterRank == 0)
            {
                VarRec->GlobalDims = meta_base->Shape;
            }
            VarRec->PerWriterStart[WriterRank] = meta_base->Offsets;
            VarRec->PerWriterCounts[WriterRank] = meta_base->Count;
            VarRec->PerWriterMetaFieldDesc[WriterRank] = &FieldList[i];
            VarRec->PerWriterDataFieldDesc[WriterRank] = NULL;
            i += 4;
        }
        else
        {
            /* simple field */
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip SST_
            FFSVarRec VarRec = NULL;
            if (WriterRank != 0)
            {
                VarRec = LookupVarByName(Stream, FieldName);
            }
            if (!VarRec)
            {
                char *Type = TranslateFFSType2ADIOS(FieldList[i].field_type,
                                                    FieldList[i].field_size);
                VarRec = CreateVarRec(Stream, FieldName);
                VarRec->DimCount = 0;
                VarRec->Variable = Stream->VarSetupUpcall(
                    Stream->SetupUpcallReader, FieldName, Type, field_data);
            }
            VarRec->PerWriterMetaFieldDesc[WriterRank] = &FieldList[i];
            VarRec->PerWriterDataFieldDesc[WriterRank] = NULL;
            i++;
        }
    }
}

extern void FFSMarshalInstallMetadata(SstStream Stream, TSMetadataMsg MetaData)
{
    if (!Stream->ReaderFFSContext)
        Stream->ReaderFFSContext =
            create_FFSContext_FM(create_local_FMcontext());

    LoadFormats(Stream, MetaData->Formats);

    for (int i = 0; i < Stream->WriterCohortSize; i++)
    {
        BuildVarList(Stream, MetaData, i);
    }
}

static void FFSBitfieldSet(struct FFSMetadataInfoStruct *MBase, int Bit)
{
    int Element = Bit / 64;
    int ElementBit = Bit % 64;
    if (Element >= MBase->BitFieldCount)
    {
        MBase->BitField =
            realloc(MBase->BitField, sizeof(size_t) * (Element + 1));
        memset(MBase->BitField + MBase->BitFieldCount * sizeof(size_t), 0,
               (Element - MBase->BitFieldCount + 1) * sizeof(size_t));
        MBase->BitFieldCount = Element + 1;
    }
    MBase->BitField[Element] |= (1 << ElementBit);
}

extern void SstMarshal(SstStream Stream, void *Variable, const char *Name,
                       const char *Type, size_t ElemSize, size_t DimCount,
                       const unsigned long *Shape, const unsigned long *Count,
                       const unsigned long *Offsets, const void *data)
{

    struct FFSMetadataInfoStruct *MBase;
    FFSWriterRec Rec = LookupWriterRec(Stream, Variable);
    if (!Rec)
    {
        Rec = CreateWriterRec(Stream, Variable, Name, Type, ElemSize, DimCount);
    }
    MBase = Stream->M;
    FFSBitfieldSet(MBase, Rec->FieldID);

    if (Rec->SingleValue)
    {
        char *base = Stream->M + Rec->MetaOffset;
        memcpy(base, data, ElemSize);
    }
    else
    {
        /* array case */
        MetaArrayRec *MetaBase = Stream->M + Rec->MetaOffset;
        ArrayRec *data_base = Stream->D + Rec->DataOffset;
        size_t ElemCount = CalcSize(DimCount, Count);
        data_base->ElemCount = ElemCount;
        /* this is PutSync case, so we have to copy the data NOW */
        data_base->Array = malloc(ElemCount * ElemSize);
        memcpy(data_base->Array, data, ElemCount * ElemSize);

        /* handle metadata */
        MetaBase->Dims = DimCount;
        MetaBase->Shape = CopyDims(DimCount, Shape);
        MetaBase->Count = CopyDims(DimCount, Count);
        MetaBase->Offsets = CopyDims(DimCount, Offsets);
    }
}
