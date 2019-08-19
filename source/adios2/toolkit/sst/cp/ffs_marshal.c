#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "adios2/common/ADIOSConfig.h"
#include <atl.h>
#include <evpath.h>
#include <pthread.h>

#include "adios2/common/ADIOSConfig.h"

#include "sst.h"

#include "adios2/toolkit/profiling/taustubs/taustubs.h"
#include "cp_internal.h"
#include "ffs_marshal.h"

typedef struct dcomplex
{
    double real_part;
    double imag_part;
} dcomplex_struct;

typedef struct fcomplex
{
    float real_part;
    float imag_part;
} fcomplex_struct;

FMField fcomplex_field_list[] = {
    {"real", "float", sizeof(float), FMOffset(fcomplex_struct *, real_part)},
    {"imag", "float", sizeof(float), FMOffset(fcomplex_struct *, imag_part)},
    {NULL, NULL, 0, 0}};

FMField dcomplex_field_list[] = {
    {"real", "float", sizeof(double), FMOffset(dcomplex_struct *, real_part)},
    {"imag", "float", sizeof(double), FMOffset(dcomplex_struct *, imag_part)},
    {NULL, NULL, 0, 0}};

static char *ConcatName(const char *base_name, const char *postfix)
{
    char *Ret =
        malloc(strlen("SST_") + strlen(base_name) + strlen(postfix) + 1);
    strcpy(Ret, "SST_");
    strcat(Ret, base_name);
    strcat(Ret, postfix);
    return Ret;
}

static char *BuildVarName(const char *base_name, const char *type,
                          const int element_size)
{
    int Len = strlen(base_name) + strlen(type) + strlen("SST_") + 16;
    char *Ret = malloc(Len);
    sprintf(Ret, "SST%d_%d_", element_size, (int)strlen(type));
    strcat(Ret, type);
    strcat(Ret, "_");
    strcat(Ret, base_name);
    return Ret;
}

static void BreakdownVarName(const char *Name, char **base_name_p,
                             char **type_p, int *element_size_p)
{
    int TypeLen;
    int ElementSize;
    const char *NameStart;
    char *TypeStart = strchr(Name, '_') + 1;
    TypeStart = strchr(TypeStart, '_') + 1;
    sscanf(Name, "SST%d_%d_", &ElementSize, &TypeLen);
    NameStart = TypeStart + TypeLen + 1;
    *element_size_p = ElementSize;
    *type_p = malloc(TypeLen + 1);
    strncpy(*type_p, TypeStart, TypeLen);
    (*type_p)[TypeLen] = 0;
    *base_name_p = strdup(NameStart);
}

static char *BuildArrayName(const char *base_name, const char *type,
                            const int element_size)
{
    int Len = strlen(base_name) + strlen(type) + strlen("SST_") + 16;
    char *Ret = malloc(Len);
    sprintf(Ret, "SST%d_%d_", element_size, (int)strlen(type));
    strcat(Ret, type);
    strcat(Ret, "_");
    strcat(Ret, base_name);
    strcat(Ret, "Dims");
    return Ret;
}

static void BreakdownArrayName(const char *Name, char **base_name_p,
                               char **type_p, int *element_size_p)
{
    int TypeLen;
    int ElementSize;
    const char *NameStart;
    char *TypeStart = strchr(Name, '_') + 1;
    TypeStart = strchr(TypeStart, '_') + 1;
    sscanf(Name, "SST%d_%d_", &ElementSize, &TypeLen);
    NameStart = TypeStart + TypeLen + 1;
    *element_size_p = ElementSize;
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
    else if (strcmp(Type, "float complex") == 0)
    {
        return strdup("complex4");
    }
    else if (strcmp(Type, "double complex") == 0)
    {
        return strdup("complex8");
    }
    else if (strcmp(Type, "int8_t") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "int16_t") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "int32_t") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "int64_t") == 0)
    {
        return strdup("integer");
    }
    else if (strcmp(Type, "uint8_t") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "uint16_t") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "uint32_t") == 0)
    {
        return strdup("unsigned integer");
    }
    else if (strcmp(Type, "uint64_t") == 0)
    {
        return strdup("unsigned integer");
    }

    return strdup(Type);
}

static char *TranslateFFSType2ADIOS(const char *Type, int size)
{
    if (strcmp(Type, "integer") == 0)
    {
        if (size == 1)
        {
            return strdup("int8_t");
        }
        else if (size == 2)
        {
            return strdup("int16_t");
        }
        else if (size == 4)
        {
            return strdup("int32_t");
        }
        else if (size == 8)
        {
            return strdup("int64_t");
        }
    }
    else if (strcmp(Type, "unsigned integer") == 0)
    {
        if (size == 1)
        {
            return strdup("uint8_t");
        }
        else if (size == 2)
        {
            return strdup("uint16_t");
        }
        else if (size == 4)
        {
            return strdup("uint32_t");
        }
        else if (size == 8)
        {
            return strdup("uint64_t");
        }
    }
    else if ((strcmp(Type, "double") == 0) || (strcmp(Type, "float") == 0))
    {
        if (size == sizeof(float))
        {
            return strdup("float");
        }
        else if ((sizeof(long double) != sizeof(double)) &&
                 (size == sizeof(long double)))
        {
            return strdup("long double");
        }
        else
        {
            return strdup("double");
        }
    }
    else if (strcmp(Type, "complex4") == 0)
    {
        return strdup("float complex");
    }
    else if (strcmp(Type, "complex8") == 0)
    {
        return strdup("double complex");
    }
    return strdup(Type);
}

static void RecalcMarshalStorageSize(SstStream Stream)
{
    struct FFSWriterMarshalBase *Info = Stream->WriterMarshalData;
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

static void RecalcAttributeStorageSize(SstStream Stream)
{
    struct FFSWriterMarshalBase *Info = Stream->WriterMarshalData;
    if (Info->AttributeFieldCount)
    {
        FMFieldList LastAttributeField;
        size_t NewAttributeSize;
        LastAttributeField =
            &Info->AttributeFields[Info->AttributeFieldCount - 1];
        NewAttributeSize = (LastAttributeField->field_offset +
                            LastAttributeField->field_size + 7) &
                           ~7;
        Info->AttributeData =
            realloc(Info->AttributeData, NewAttributeSize + 8);
        memset(Info->AttributeData + Info->AttributeSize, 0,
               NewAttributeSize - Info->AttributeSize);
        Info->AttributeSize = NewAttributeSize;
    }
}

static void AddSimpleField(FMFieldList *FieldP, int *CountP, const char *Name,
                           const char *Type, int ElementSize)
{
    int Offset = 0;
    FMFieldList Field;
    if (*CountP)
    {
        FMFieldList PriorField;
        PriorField = &((*FieldP)[(*CountP) - 1]);
        int PriorFieldSize = PriorField->field_size;
        if (strchr(PriorField->field_type, '['))
        {
            // really a pointer
            PriorFieldSize = sizeof(void *);
        }
        Offset =
            ((PriorField->field_offset + PriorFieldSize + ElementSize - 1) /
             ElementSize) *
            ElementSize;
    }
    *FieldP = realloc(*FieldP, (*CountP + 2) * sizeof((*FieldP)[0]));
    Field = &((*FieldP)[*CountP]);
    (*CountP)++;
    Field->field_name = strdup(Name);
    Field->field_type = strdup(Type);
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
    char *TransType = TranslateADIOS2Type2FFS(Type);
    AddSimpleField(FieldP, CountP, Name, TransType, ElementSize);
    free(TransType);
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
    free(TypeWithArray);
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
    free(TypeWithArray);
    (*FieldP)[*CountP - 1].field_size = ElementSize;
}

struct FFSMetadataInfoStruct
{
    size_t BitFieldCount;
    size_t *BitField;
    size_t DataBlockSize;
};

static int FFSBitfieldTest(struct FFSMetadataInfoStruct *MBase, int Bit);

static void InitMarshalData(SstStream Stream)
{
    struct FFSWriterMarshalBase *Info =
        malloc(sizeof(struct FFSWriterMarshalBase));
    struct FFSMetadataInfoStruct *MBase;

    memset(Info, 0, sizeof(*Info));
    Stream->WriterMarshalData = Info;
    Info->RecCount = 0;
    Info->RecList = malloc(sizeof(Info->RecList[0]));
    Info->MetaFieldCount = 0;
    Info->MetaFields = malloc(sizeof(Info->MetaFields[0]));
    Info->DataFieldCount = 0;
    Info->DataFields = malloc(sizeof(Info->DataFields[0]));
    Info->LocalFMContext = create_local_FMcontext();
    AddSimpleField(&Info->MetaFields, &Info->MetaFieldCount, "BitFieldCount",
                   "integer", sizeof(size_t));
    AddSimpleField(&Info->MetaFields, &Info->MetaFieldCount, "BitField",
                   "integer[BitFieldCount]", sizeof(size_t));
    AddSimpleField(&Info->MetaFields, &Info->MetaFieldCount, "DataBlockSize",
                   "integer", sizeof(size_t));
    RecalcMarshalStorageSize(Stream);
    MBase = Stream->M;
    MBase->BitFieldCount = 0;
    MBase->BitField = malloc(sizeof(size_t));
    MBase->DataBlockSize = 0;
}

extern void FFSFreeMarshalData(SstStream Stream)
{
    if (Stream->Role == WriterRole)
    {
        /* writer side */
        struct FFSWriterMarshalBase *Info =
            (struct FFSWriterMarshalBase *)Stream->WriterMarshalData;
        struct FFSMetadataInfoStruct *MBase;
        MBase = Stream->M;

        for (int i = 0; i < Info->RecCount; i++)
        {
            free(Info->RecList[i].Type);
        }
        if (Info->RecList)
            free(Info->RecList);
        if (Info->MetaFields)
            free_FMfield_list(Info->MetaFields);
        if (Info->DataFields)
            free_FMfield_list(Info->DataFields);
        if (Info->LocalFMContext)
            free_FMcontext(Info->LocalFMContext);
        free(Info);
        Stream->WriterMarshalData = NULL;
        free(Stream->D);
        Stream->D = NULL;
        free(MBase->BitField);
        free(Stream->M);
        Stream->M = NULL;
    }
    else
    {
        /* reader side */
        struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;
        if (Info)
        {
            for (int i = 0; i < Stream->WriterCohortSize; i++)
            {
                if (Info->WriterInfo[i].RawBuffer)
                    free(Info->WriterInfo[i].RawBuffer);
            }
            if (Info->WriterInfo)
                free(Info->WriterInfo);
            if (Info->MetadataBaseAddrs)
                free(Info->MetadataBaseAddrs);
            if (Info->MetadataFieldLists)
                free(Info->MetadataFieldLists);
            if (Info->DataBaseAddrs)
                free(Info->DataBaseAddrs);
            if (Info->DataFieldLists)
                free(Info->DataFieldLists);
            for (int i = 0; i < Info->VarCount; i++)
            {
                free(Info->VarList[i].VarName);
                free(Info->VarList[i].PerWriterMetaFieldDesc);
                free(Info->VarList[i].PerWriterDataFieldDesc);
                free(Info->VarList[i].PerWriterStart);
                free(Info->VarList[i].PerWriterCounts);
                free(Info->VarList[i].PerWriterIncomingData);
                free(Info->VarList[i].PerWriterIncomingSize);
            }
            if (Info->VarList)
                free(Info->VarList);

            free(Info);
            Stream->ReaderMarshalData = NULL;
        }
    }
}

#if !defined(ADIOS2_HAVE_ZFP)
#define ZFPcompressionPossible(Type, DimCount) 0
#endif

static FFSWriterRec CreateWriterRec(SstStream Stream, void *Variable,
                                    const char *Name, const char *Type,
                                    size_t ElemSize, size_t DimCount)
{
    if (!Stream->WriterMarshalData)
    {
        InitMarshalData(Stream);
    }
    struct FFSWriterMarshalBase *Info =
        (struct FFSWriterMarshalBase *)Stream->WriterMarshalData;
    Info->RecList =
        realloc(Info->RecList, (Info->RecCount + 1) * sizeof(Info->RecList[0]));
    FFSWriterRec Rec = &Info->RecList[Info->RecCount];
    Rec->Key = Variable;
    Rec->FieldID = Info->RecCount;
    Rec->DimCount = DimCount;
    Rec->Type = strdup(Type);
    if (DimCount == 0)
    {
        // simple field, only add base value FMField to metadata
        char *SstName = ConcatName(Name, "");
        AddField(&Info->MetaFields, &Info->MetaFieldCount, SstName, Type,
                 ElemSize);
        free(SstName);
        RecalcMarshalStorageSize(Stream);
        Rec->MetaOffset =
            Info->MetaFields[Info->MetaFieldCount - 1].field_offset;
        Rec->DataOffset = (size_t)-1;
        // Changing the formats renders these invalid
        Info->MetaFormat = NULL;
    }
    else
    {
        // Array field.  To Metadata, add FMFields for DimCount, Shape, Count
        // and Offsets matching _MetaArrayRec
        char *ArrayName = BuildArrayName(Name, Type, ElemSize);
        AddField(&Info->MetaFields, &Info->MetaFieldCount, ArrayName, "integer",
                 sizeof(size_t));
        free(ArrayName);
        Rec->MetaOffset =
            Info->MetaFields[Info->MetaFieldCount - 1].field_offset;
        char *ShapeName = ConcatName(Name, "Shape");
        char *CountName = ConcatName(Name, "Count");
        char *OffsetsName = ConcatName(Name, "Offsets");
        AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount, ShapeName,
                           "integer", sizeof(size_t), DimCount);
        AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount, CountName,
                           "integer", sizeof(size_t), DimCount);
        AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount,
                           OffsetsName, "integer", sizeof(size_t), DimCount);
        free(ShapeName);
        free(CountName);
        free(OffsetsName);
        RecalcMarshalStorageSize(Stream);

        if ((Stream->ConfigParams->CompressionMethod == SstCompressZFP) &&
            ZFPcompressionPossible(Type, DimCount))
        {
            Type = "char";
            ElemSize = 1;
        }
        // To Data, add FMFields for ElemCount and Array matching _ArrayRec
        char *ElemCountName = ConcatName(Name, "ElemCount");
        AddField(&Info->DataFields, &Info->DataFieldCount, ElemCountName,
                 "integer", sizeof(size_t));
        Rec->DataOffset =
            Info->DataFields[Info->DataFieldCount - 1].field_offset;
        char *SstName = ConcatName(Name, "");
        AddVarArrayField(&Info->DataFields, &Info->DataFieldCount, SstName,
                         Type, ElemSize, ElemCountName);
        free(SstName);
        free(ElemCountName);
        RecalcMarshalStorageSize(Stream);
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

static void FreeAttrInfo(void *ClientData)
{
    free_FFSBuffer((FFSBuffer)ClientData);
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
    struct FFSWriterMarshalBase *Info = Stream->WriterMarshalData;

    if (!Stream->WriterMarshalData)
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
    memset(&Info->VarList[Info->VarCount], 0, sizeof(Info->VarList[0]));
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
    Info->VarList[Info->VarCount].PerWriterIncomingSize =
        calloc(sizeof(size_t), Stream->WriterCohortSize);
    return &Info->VarList[Info->VarCount++];
}

extern void CP_verbose(SstStream Stream, char *Format, ...);

extern int SstFFSWriterBeginStep(SstStream Stream, int mode,
                                 const float timeout_sec)
{
    return 0;
}

void SstReaderInitFFSCallback(SstStream Stream, void *Reader,
                              VarSetupUpcallFunc VarCallback,
                              ArraySetupUpcallFunc ArrayCallback,
                              AttrSetupUpcallFunc AttrCallback,
                              ArrayBlocksInfoUpcallFunc BlocksInfoCallback)
{
    Stream->VarSetupUpcall = VarCallback;
    Stream->ArraySetupUpcall = ArrayCallback;
    Stream->AttrSetupUpcall = AttrCallback;
    Stream->ArrayBlocksInfoUpcall = BlocksInfoCallback;
    Stream->SetupUpcallReader = Reader;
}

extern void SstFFSGetDeferred(SstStream Stream, void *Variable,
                              const char *Name, size_t DimCount,
                              const size_t *Start, const size_t *Count,
                              void *Data)
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
        Req->RequestType = Global;
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

extern void SstFFSGetLocalDeferred(SstStream Stream, void *Variable,
                                   const char *Name, size_t DimCount,
                                   const int BlockID, const size_t *Count,
                                   void *Data)
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
        memset(Req, 0, sizeof(*Req));
        Req->VarRec = Var;
        Req->RequestType = Local;
        Req->NodeID = BlockID;
        // make a copy of Count request
        Req->Count = malloc(sizeof(Count[0]) * Var->DimCount);
        memcpy(Req->Count, Count, sizeof(Count[0]) * Var->DimCount);
        Req->Data = Data;
        Req->Next = Info->PendingVarRequests;
        Info->PendingVarRequests = Req;
    }
}

static int NeedWriter(FFSArrayRequest Req, int i)
{
    if (Req->RequestType == Local)
    {
        return (Req->NodeID == i);
    }
    // else Global case
    for (int j = 0; j < Req->VarRec->DimCount; j++)
    {
        size_t SelOffset = Req->Start[j];
        size_t SelSize = Req->Count[j];
        size_t RankOffset;
        size_t RankSize;
        if (Req->VarRec->PerWriterStart[i] == NULL)
        /* this writer didn't write */
        {
            return 0;
        }
        RankOffset = Req->VarRec->PerWriterStart[i][j];
        RankSize = Req->VarRec->PerWriterCounts[i][j];
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
    SstFullMetadata Mdata = Stream->CurrentMetadata;

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
            void *DP_TimestepInfo =
                Mdata->DP_TimestepInfo ? Mdata->DP_TimestepInfo[i] : NULL;
            Info->WriterInfo[i].RawBuffer =
                realloc(Info->WriterInfo[i].RawBuffer, DataSize);

            char tmpstr[256] = {0};
            sprintf(tmpstr, "Request to rank %d, bytes", i);
            TAU_SAMPLE_COUNTER(tmpstr, (double)DataSize);
            Info->WriterInfo[i].ReadHandle = SstReadRemoteMemory(
                Stream, i, Stream->ReaderTimestep, 0, DataSize,
                Info->WriterInfo[i].RawBuffer, DP_TimestepInfo);
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
        FFSArrayRequest PrevReq = Req;
        Req = Req->Next;
        free(PrevReq->Count);
        free(PrevReq->Start);
        free(PrevReq);
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
    int DumpData = -1;

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
        FMfree_struct_list(List);
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
    if (DumpData == -1)
    {
        DumpData = (getenv("SstDumpData") != NULL);
    }
    if (DumpData)
    {
        printf("\nOn Rank %d, IncomingDatablock from writer %d is %p :\n",
               Stream->Rank, Writer, BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
    }
    Info->DataBaseAddrs[Writer] = BaseData;
    FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
    FieldList = FormatList[0].field_list;
    Info->DataFieldLists[Writer] = FieldList;

    int i = 0;
    while (FieldList[i].field_name)
    {
        ArrayRec *data_base =
            (ArrayRec *)((char *)BaseData + FieldList[i].field_offset);
        const char *ArrayName = FieldList[i + 1].field_name + 4;
        FFSVarRec VarRec = LookupVarByName(Stream, ArrayName);
        if (VarRec)
        {
            VarRec->PerWriterIncomingData[Writer] = data_base->Array;
            VarRec->PerWriterIncomingSize[Writer] = data_base->ElemCount;
            VarRec->PerWriterDataFieldDesc[Writer] = &FieldList[i + 1];
        }
        i += 2;
    }
}

static SstStatusValue WaitForReadRequests(SstStream Stream)
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
                CP_verbose(Stream, "Wait for remote read completion failed, "
                                   "returning failure\n");
                return Result;
            }
        }
    }
    CP_verbose(Stream, "All remote memory reads completed\n");
    return SstSuccess;
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

static int FindOffsetCM(size_t Dims, const size_t *Size, const size_t *Index)
{
    int Offset = 0;
    for (int i = Dims - 1; i >= 0; i--)
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
// Row major version
void ExtractSelectionFromPartialRM(int ElementSize, size_t Dims,
                                   const size_t *GlobalDims,
                                   const size_t *PartialOffsets,
                                   const size_t *PartialCounts,
                                   const size_t *SelectionOffsets,
                                   const size_t *SelectionCounts,
                                   const char *InData, char *OutData)
{
    size_t BlockSize;
    size_t SourceBlockStride = 0;
    size_t DestBlockStride = 0;
    size_t SourceBlockStartOffset;
    size_t DestBlockStartOffset;
    size_t BlockCount;
    size_t OperantDims;
    size_t OperantElementSize;

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
            size_t Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
            size_t Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                               SelectionOffsets[Dim] + SelectionCounts[Dim]);
            BlockSize *= (Right - Left);
            break;
        }
    }
    if (OperantDims > 0)
    {
        SourceBlockStride = PartialCounts[OperantDims - 1] * OperantElementSize;
        DestBlockStride = SelectionCounts[OperantDims - 1] * OperantElementSize;
    }

    /* calculate first selected element and count */
    BlockCount = 1;
    size_t *FirstIndex = malloc(Dims * sizeof(FirstIndex[0]));
    for (int Dim = 0; Dim < Dims; Dim++)
    {
        size_t Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
        size_t Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
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
    size_t i;
    for (i = 0; i < BlockCount; i++)
    {
        memcpy(OutData, InData, BlockSize * ElementSize);
        InData += SourceBlockStride;
        OutData += DestBlockStride;
    }
    free(FirstIndex);
}

static void ReverseDimensions(size_t *Dimensions, int count)
{
    for (int i = 0; i < count / 2; i++)
    {
        size_t tmp = Dimensions[i];
        Dimensions[i] = Dimensions[count - i - 1];
        Dimensions[count - i - 1] = tmp;
    }
}

// Column-major version
void ExtractSelectionFromPartialCM(int ElementSize, size_t Dims,
                                   const size_t *GlobalDims,
                                   const size_t *PartialOffsets,
                                   const size_t *PartialCounts,
                                   const size_t *SelectionOffsets,
                                   const size_t *SelectionCounts,
                                   const char *InData, char *OutData)
{
    int BlockSize;
    int SourceBlockStride = 0;
    int DestBlockStride = 0;
    int SourceBlockStartOffset;
    int DestBlockStartOffset;
    int BlockCount;
    int OperantElementSize;

    BlockSize = 1;
    OperantElementSize = ElementSize;
    for (int Dim = 0; Dim < Dims; Dim++)
    {
        if ((GlobalDims[Dim] == PartialCounts[Dim]) &&
            (SelectionCounts[Dim] == PartialCounts[Dim]))
        {
            BlockSize *= GlobalDims[Dim];
            OperantElementSize *= GlobalDims[Dim];
            /* skip the first bit of everything */
            GlobalDims++;
            PartialOffsets++;
            PartialCounts++;
            SelectionOffsets++;
            SelectionCounts++;
            Dims--;
            /* and make sure we do the next dimensions appropriately by
             * repeating this iterator value */
            Dim--;
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
    if (Dims > 0)
    {
        SourceBlockStride = PartialCounts[0] * OperantElementSize;
        DestBlockStride = SelectionCounts[0] * OperantElementSize;
    }

    /* calculate first selected element and count */
    BlockCount = 1;
    size_t *FirstIndex = malloc(Dims * sizeof(FirstIndex[0]));
    for (int Dim = 0; Dim < Dims; Dim++)
    {
        int Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
        int Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                        SelectionOffsets[Dim] + SelectionCounts[Dim]);
        if (Dim > 0)
        {
            BlockCount *= (Right - Left);
        }
        FirstIndex[Dim] = Left;
    }
    size_t *SelectionIndex = malloc(Dims * sizeof(SelectionIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, SelectionOffsets, SelectionIndex);
    DestBlockStartOffset = FindOffsetCM(Dims, SelectionCounts, SelectionIndex);
    free(SelectionIndex);
    DestBlockStartOffset *= OperantElementSize;

    size_t *PartialIndex = malloc(Dims * sizeof(PartialIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, PartialOffsets, PartialIndex);
    SourceBlockStartOffset = FindOffsetCM(Dims, PartialCounts, PartialIndex);

    free(PartialIndex);
    SourceBlockStartOffset *= OperantElementSize;

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

typedef struct _range_list
{
    size_t start;
    size_t end;
    struct _range_list *next;
} * range_list;

range_list static OneDCoverage(size_t start, size_t end,
                               range_list uncovered_list)
{
    if (uncovered_list == NULL)
        return NULL;

    if ((start <= uncovered_list->start) && (end >= uncovered_list->end))
    {
        /* this uncovered element is covered now, recurse on next */
        range_list next = uncovered_list->next;
        free(uncovered_list);
        return OneDCoverage(start, end, next);
    }
    else if ((end < uncovered_list->end) && (start > uncovered_list->start))
    {
        /* covering a bit in the middle */
        range_list new = malloc(sizeof(*new));
        new->next = uncovered_list->next;
        new->end = uncovered_list->end;
        new->start = end + 1;
        uncovered_list->end = start - 1;
        uncovered_list->next = new;
        return (uncovered_list);
    }
    else if ((end < uncovered_list->start) || (start > uncovered_list->end))
    {
        uncovered_list->next = OneDCoverage(start, end, uncovered_list->next);
        return uncovered_list;
    }
    else if (start <= uncovered_list->start)
    {
        /* we don't cover completely nor a middle portion, so this means we span
         * the beginning */
        uncovered_list->start = end + 1;
        uncovered_list->next = OneDCoverage(start, end, uncovered_list->next);
        return uncovered_list;
    }
    else if (end >= uncovered_list->end)
    {
        /* we don't cover completely nor a middle portion, so this means we span
         * the end */
        uncovered_list->end = start - 1;
        uncovered_list->next = OneDCoverage(start, end, uncovered_list->next);
        return uncovered_list;
    }
    return NULL;
}

static void DumpCoverageList(range_list list)
{
    if (!list)
        return;
    printf("%ld - %ld", list->start, list->end);
    if (list->next != NULL)
    {
        printf(", ");
        DumpCoverageList(list->next);
    }
}

static void ImplementGapWarning(SstStream Stream, FFSArrayRequest Req)
{
    if (Req->RequestType == Local)
    {
        /* no analysis here */
        return;
    }
    if (Req->VarRec->DimCount != 1)
    {
        /* at this point, multidimensional fill analysis is too much */
        return;
    }
    struct _range_list *Required = malloc(sizeof(*Required));
    Required->next = NULL;
    Required->start = Req->Start[0];
    Required->end = Req->Start[0] + Req->Count[0] - 1;
    for (int i = 0; i < Stream->WriterCohortSize; i++)
    {
        size_t start = Req->VarRec->PerWriterStart[i][0];
        size_t end = start + Req->VarRec->PerWriterCounts[i][0] - 1;
        Required = OneDCoverage(start, end, Required);
    }
    if (Required != NULL)
    {
        printf("WARNING:   Reader Rank %d requested elements %lu - %lu,\n\tbut "
               "these elements were not written by any writer rank: \n",
               Stream->Rank, (unsigned long)Req->Start[0],
               (unsigned long)Req->Start[0] + Req->Count[0] - 1);
        DumpCoverageList(Required);
    }
}

static void FillReadRequests(SstStream Stream, FFSArrayRequest Reqs)
{
    while (Reqs)
    {
        ImplementGapWarning(Stream, Reqs);
        for (int i = 0; i < Stream->WriterCohortSize; i++)
        {
            if (NeedWriter(Reqs, i))
            {
                /* if needed this writer fill destination with acquired data */
                int ElementSize = Reqs->VarRec->ElementSize;
                int DimCount = Reqs->VarRec->DimCount;
                size_t *GlobalDimensions = Reqs->VarRec->GlobalDims;
                size_t *RankOffset = Reqs->VarRec->PerWriterStart[i];
                size_t *RankSize = Reqs->VarRec->PerWriterCounts[i];
                size_t *SelOffset = Reqs->Start;
                size_t *SelSize = Reqs->Count;
                char *Type = Reqs->VarRec->Type;
                void *IncomingData = Reqs->VarRec->PerWriterIncomingData[i];
                size_t IncomingSize = Reqs->VarRec->PerWriterIncomingSize[i];
                int FreeIncoming = 0;

                if (Reqs->RequestType == Local)
                {
                    RankOffset = calloc(DimCount, sizeof(RankOffset[0]));
                    GlobalDimensions =
                        calloc(DimCount, sizeof(GlobalDimensions[0]));
                    if (SelOffset == NULL)
                    {
                        SelOffset = calloc(DimCount, sizeof(RankOffset[0]));
                    }
                    for (int i = 0; i < DimCount; i++)
                    {
                        GlobalDimensions[i] = RankSize[i];
                    }
                }
                if ((Stream->WriterConfigParams->CompressionMethod ==
                     SstCompressZFP) &&
                    ZFPcompressionPossible(Type, DimCount))
                {
#ifdef ADIOS2_HAVE_ZFP
                    /*
                     * replace old IncomingData with uncompressed, and free
                     * afterwards
                     */
                    FreeIncoming = 1;
                    IncomingData =
                        FFS_ZFPDecompress(Stream, DimCount, Type, IncomingData,
                                          IncomingSize, RankSize, NULL);
#endif
                }
                if (Stream->ConfigParams->IsRowMajor)
                {
                    ExtractSelectionFromPartialRM(
                        ElementSize, DimCount, GlobalDimensions, RankOffset,
                        RankSize, SelOffset, SelSize, IncomingData, Reqs->Data);
                }
                else
                {
                    ExtractSelectionFromPartialCM(
                        ElementSize, DimCount, GlobalDimensions, RankOffset,
                        RankSize, SelOffset, SelSize, IncomingData, Reqs->Data);
                }
                if (FreeIncoming)
                {
                    /* free uncompressed  */
                    free(IncomingData);
                }
            }
        }
        Reqs = Reqs->Next;
    }
}

extern SstStatusValue SstFFSPerformGets(SstStream Stream)
{
    struct FFSReaderMarshalBase *Info = Stream->ReaderMarshalData;
    SstStatusValue Ret;

    IssueReadRequests(Stream, Info->PendingVarRequests);

    Ret = WaitForReadRequests(Stream);

    if (Ret == SstSuccess)
    {
        FillReadRequests(Stream, Info->PendingVarRequests);
    }
    else
    {
        CP_verbose(Stream, "Some memory read failed, not filling requests and "
                           "returning failure\n");
    }
    ClearReadRequests(Stream);

    return Ret;
}

extern void SstFFSWriterEndStep(SstStream Stream, size_t Timestep)
{
    struct FFSWriterMarshalBase *Info =
        (struct FFSWriterMarshalBase *)Stream->WriterMarshalData;
    struct FFSFormatBlock *Formats = NULL;
    FMFormat AttributeFormat = NULL;

    TAU_START("Marshaling overhead in SstFFSWriterEndStep");

    CP_verbose(Stream, "Calling SstWriterEndStep\n");
    // if field lists have changed, register formats with FFS local context, add
    // to format chain
    if (!Info->MetaFormat)
    {
        struct FFSFormatBlock *Block = malloc(sizeof(*Block));
        FMStructDescRec struct_list[4] = {
            {NULL, NULL, 0, NULL},
            {"complex4", fcomplex_field_list, sizeof(fcomplex_struct), NULL},
            {"complex8", dcomplex_field_list, sizeof(dcomplex_struct), NULL},
            {NULL, NULL, 0, NULL}};
        struct_list[0].format_name = "MetaData";
        struct_list[0].field_list = Info->MetaFields;
        struct_list[0].struct_size =
            FMstruct_size_field_list(Info->MetaFields, sizeof(char *));
        FMFormat Format =
            register_data_format(Info->LocalFMContext, &struct_list[0]);
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
        FMStructDescRec struct_list[4] = {
            {NULL, NULL, 0, NULL},
            {"complex4", fcomplex_field_list, sizeof(fcomplex_struct), NULL},
            {"complex8", dcomplex_field_list, sizeof(dcomplex_struct), NULL},
            {NULL, NULL, 0, NULL}};
        struct_list[0].format_name = "Data";
        struct_list[0].field_list = Info->DataFields;
        struct_list[0].struct_size =
            FMstruct_size_field_list(Info->DataFields, sizeof(char *));
        FMFormat Format =
            register_data_format(Info->LocalFMContext, &struct_list[0]);
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
    if (Info->AttributeFields)
    {
        struct FFSFormatBlock *Block = malloc(sizeof(*Block));
        FMFormat Format = FMregister_simple_format(
            Info->LocalFMContext, "Attributes", Info->AttributeFields,
            FMstruct_size_field_list(Info->AttributeFields, sizeof(char *)));
        AttributeFormat = Format;
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
    FFSBuffer AttributeEncodeBuffer = NULL;
    struct _SstData DataRec;
    struct _SstData MetaDataRec;
    struct _SstData AttributeRec;
    int MetaDataSize;
    int DataSize;
    int AttributeSize = 0;
    struct FFSMetadataInfoStruct *MBase;
    DataRec.block =
        FFSencode(DataEncodeBuffer, Info->DataFormat, Stream->D, &DataSize);
    DataRec.DataSize = DataSize;
    TSInfo->DataEncodeBuffer = DataEncodeBuffer;

    MBase = Stream->M;
    MBase->DataBlockSize = DataSize;
    MetaDataRec.block =
        FFSencode(MetaEncodeBuffer, Info->MetaFormat, Stream->M, &MetaDataSize);
    MetaDataRec.DataSize = MetaDataSize;
    TSInfo->MetaEncodeBuffer = MetaEncodeBuffer;

    if (Info->AttributeFields)
    {
        AttributeEncodeBuffer = create_FFSBuffer();
        AttributeRec.block = FFSencode(AttributeEncodeBuffer, AttributeFormat,
                                       Info->AttributeData, &AttributeSize);
        AttributeRec.DataSize = AttributeSize;
    }
    else
    {
        AttributeRec.block = NULL;
        AttributeRec.DataSize = 0;
    }

    /* free all those copied dimensions, etc */
    MBase = Stream->M;
    size_t *tmp = MBase->BitField;
    /*
     * BitField value is saved away from FMfree_var_rec_elements() so that it
     * isn't unnecessarily free'd.
     */
    MBase->BitField = NULL;
    FMfree_var_rec_elements(Info->MetaFormat, Stream->M);
    MBase->BitField = tmp;
    FMfree_var_rec_elements(Info->DataFormat, Stream->D);
    memset(Stream->M, 0, Stream->MetadataSize);
    memset(Stream->D, 0, Stream->DataSize);

    // Call SstInternalProvideStep with Metadata block, Data block and (any new)
    // formatID and formatBody
    //    printf("MetaDatablock is (Length %d):\n", MetaDataSize);
    //    FMdump_encoded_data(Info->MetaFormat, MetaDataRec.block, 1024000);
    //    printf("\nDatablock is :\n");
    //    FMdump_encoded_data(Info->DataFormat, DataRec.block, 1024000);
    //    if (AttributeEncodeBuffer) {
    //        printf("\nAttributeBlock is :\n");
    //        FMdump_encoded_data(AttributeFormat, AttributeRec.block, 1024000);
    //    }

    TAU_STOP("Marshaling overhead in SstFFSWriterEndStep");

    SstInternalProvideTimestep(Stream, &MetaDataRec, &DataRec, Timestep,
                               Formats, FreeTSInfo, TSInfo, &AttributeRec,
                               FreeAttrInfo, AttributeEncodeBuffer);
    while (Formats)
    {
        struct FFSFormatBlock *Tmp = Formats->Next;
        free(Formats);
        Formats = Tmp;
    }
    if (Info->AttributeFields)
        free_FMfield_list(Info->AttributeFields);
    Info->AttributeFields = NULL;
    Info->AttributeFieldCount = 0;
    if (Info->AttributeData)
        free(Info->AttributeData);
    Info->AttributeData = NULL;
    Info->AttributeSize = 0;
}

static void LoadAttributes(SstStream Stream, TSMetadataMsg MetaData)
{
    static int DumpMetadata = -1;
    Stream->AttrSetupUpcall(Stream->SetupUpcallReader, NULL, NULL, NULL);
    for (int WriterRank = 0; WriterRank < Stream->WriterCohortSize;
         WriterRank++)
    {
        FMFieldList FieldList;
        FMStructDescList FormatList;
        void *BaseData;
        FFSTypeHandle FFSformat;

        if (MetaData->AttributeData[WriterRank].DataSize == 0)
            return;

        FFSformat = FFSTypeHandle_from_encode(
            Stream->ReaderFFSContext,
            MetaData->AttributeData[WriterRank].block);
        if (!FFShas_conversion(FFSformat))
        {
            FMContext FMC = FMContext_from_FFS(Stream->ReaderFFSContext);
            FMFormat Format = FMformat_from_ID(
                FMC, MetaData->AttributeData[WriterRank].block);
            FMStructDescList List =
                FMcopy_struct_list(format_list_of_FMFormat(Format));
            FMlocalize_structs(List);
            establish_conversion(Stream->ReaderFFSContext, FFSformat, List);
            FMfree_struct_list(List);
        }

        if (FFSdecode_in_place_possible(FFSformat))
        {
            FFSdecode_in_place(Stream->ReaderFFSContext,
                               MetaData->AttributeData[WriterRank].block,
                               &BaseData);
        }
        else
        {
            int DecodedLength = FFS_est_decode_length(
                Stream->ReaderFFSContext,
                MetaData->AttributeData[WriterRank].block,
                MetaData->AttributeData[WriterRank].DataSize);
            BaseData = malloc(DecodedLength);
            FFSBuffer decode_buf =
                create_fixed_FFSBuffer(BaseData, DecodedLength);
            FFSdecode_to_buffer(Stream->ReaderFFSContext,
                                MetaData->AttributeData[WriterRank].block,
                                decode_buf);
        }
        if (DumpMetadata == -1)
        {
            DumpMetadata = (getenv("SstDumpMetadata") != NULL);
        }
        if (DumpMetadata && (Stream->Rank == 0))
        {
            printf("\nIncomingAttributeDatablock from WriterRank %d is %p :\n",
                   WriterRank, BaseData);
            FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
            printf("\n\n");
        }
        FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
        FieldList = FormatList[0].field_list;
        int i = 0;
        int j = 0;
        while (FieldList[i].field_name)
        {
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip SST_
            void *field_data = (char *)BaseData + FieldList[i].field_offset;

            char *Type;
            int ElemSize;
            BreakdownVarName(FieldList[i].field_name, &FieldName, &Type,
                             &ElemSize);
            Stream->AttrSetupUpcall(Stream->SetupUpcallReader, FieldName, Type,
                                    field_data);
            i++;
        }
    }
}

static void LoadFormats(SstStream Stream, FFSFormatList Formats)
{
    FFSFormatList Entry = Formats;
    while (Entry)
    {
        char *FormatID = malloc(Entry->FormatIDRepLen);
        char *FormatServerRep = malloc(Entry->FormatServerRepLen);
        memcpy(FormatID, Entry->FormatIDRep, Entry->FormatIDRepLen);
        memcpy(FormatServerRep, Entry->FormatServerRep,
               Entry->FormatServerRepLen);
        load_external_format_FMcontext(
            FMContext_from_FFS(Stream->ReaderFFSContext), FormatID,
            Entry->FormatIDRepLen, FormatServerRep);
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
    for (int i = 0; i < Stream->WriterCohortSize; i++)
    {
        if (Info->WriterInfo[i].RawBuffer)
            free(Info->WriterInfo[i].RawBuffer);
    }
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
        free(Info->VarList[i].PerWriterIncomingSize);
        if (Info->VarList[i].Type)
            free(Info->VarList[i].Type);
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
    static int DumpMetadata = -1;

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

    if (!MetaData->Metadata[WriterRank].block)
    {
        fprintf(stderr,
                "FAILURE!   MetaData->Metadata[WriterRank]->block == "
                "NULL for WriterRank = %d\n",
                WriterRank);
    }
    FFSformat = FFSTypeHandle_from_encode(Stream->ReaderFFSContext,
                                          MetaData->Metadata[WriterRank].block);

    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(Stream->ReaderFFSContext);
        FMFormat Format =
            FMformat_from_ID(FMC, MetaData->Metadata[WriterRank].block);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        FMlocalize_structs(List);
        establish_conversion(Stream->ReaderFFSContext, FFSformat, List);
        FMfree_struct_list(List);
    }

    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(Stream->ReaderFFSContext,
                           MetaData->Metadata[WriterRank].block, &BaseData);
    }
    else
    {
        int DecodedLength = FFS_est_decode_length(
            Stream->ReaderFFSContext, MetaData->Metadata[WriterRank].block,
            MetaData->Metadata[WriterRank].DataSize);
        BaseData = malloc(DecodedLength);
        FFSBuffer decode_buf = create_fixed_FFSBuffer(BaseData, DecodedLength);
        FFSdecode_to_buffer(Stream->ReaderFFSContext,
                            MetaData->Metadata[WriterRank].block, decode_buf);
    }
    if (DumpMetadata == -1)
    {
        DumpMetadata = (getenv("SstDumpMetadata") != NULL);
    }
    if (DumpMetadata && (Stream->Rank == 0))
    {
        printf("\nIncomingMetadatablock from WriterRank %d is %p :\n",
               WriterRank, BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
        printf("\n\n");
    }
    Info->MetadataBaseAddrs[WriterRank] = BaseData;
    FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
    FieldList = FormatList[0].field_list;
    while (strncmp(FieldList->field_name, "BitField", 8) == 0)
        FieldList++;
    while (strncmp(FieldList->field_name, "DataBlockSize", 8) == 0)
        FieldList++;
    int i = 0;
    int j = 0;
    while (FieldList[i].field_name)
    {
        void *field_data = (char *)BaseData + FieldList[i].field_offset;
        if (NameIndicatesArray(FieldList[i].field_name))
        {
            MetaArrayRec *meta_base = field_data;
            char *ArrayName;
            char *Type;
            FFSVarRec VarRec = NULL;
            int ElementSize;
            if (!FFSBitfieldTest(BaseData, j))
            {
                /* only work with fields that were written */
                i += 4;
                j++;
                continue;
            }
            BreakdownArrayName(FieldList[i].field_name, &ArrayName, &Type,
                               &ElementSize);
            if (WriterRank != 0)
            {
                VarRec = LookupVarByName(Stream, ArrayName);
            }
            if ((meta_base->Dims > 1) &&
                (Stream->WriterConfigParams->IsRowMajor !=
                 Stream->ConfigParams->IsRowMajor))
            {
                /* if we're getting data from someone of the other array gender,
                 * switcheroo */
                ReverseDimensions(meta_base->Shape, meta_base->Dims);
                ReverseDimensions(meta_base->Count, meta_base->Dims);
                ReverseDimensions(meta_base->Offsets, meta_base->Dims);
            }
            if (!VarRec)
            {
                VarRec = CreateVarRec(Stream, ArrayName);
                VarRec->DimCount = meta_base->Dims;
                VarRec->Type = Type;
                VarRec->ElementSize = ElementSize;
                VarRec->Variable = Stream->ArraySetupUpcall(
                    Stream->SetupUpcallReader, ArrayName, Type, meta_base->Dims,
                    meta_base->Shape, meta_base->Offsets, meta_base->Count);
            }
            if (WriterRank == 0)
            {
                VarRec->GlobalDims = meta_base->Shape;
            }
            VarRec->PerWriterStart[WriterRank] = meta_base->Offsets;
            VarRec->PerWriterCounts[WriterRank] = meta_base->Count;
            VarRec->PerWriterMetaFieldDesc[WriterRank] = &FieldList[i];
            VarRec->PerWriterDataFieldDesc[WriterRank] = NULL;
            Stream->ArrayBlocksInfoUpcall(Stream->SetupUpcallReader,
                                          VarRec->Variable, Type, WriterRank,
                                          meta_base->Dims, meta_base->Shape,
                                          meta_base->Offsets, meta_base->Count);
            i += 4;
            free(ArrayName);
        }
        else
        {
            /* simple field */
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip SST_
            FFSVarRec VarRec = NULL;
            if (!FFSBitfieldTest(BaseData, j))
            {
                /* only work with fields that were written */
                i++;
                j++;
                continue;
            }
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
                free(Type);
            }
            VarRec->PerWriterMetaFieldDesc[WriterRank] = &FieldList[i];
            VarRec->PerWriterDataFieldDesc[WriterRank] = NULL;
            free(FieldName);
            i++;
        }
        /* real variable count is in j, i tracks the entries in the metadata */
        j++;
    }
}

extern void FFSMarshalInstallPreciousMetadata(SstStream Stream,
                                              TSMetadataMsg MetaData)
{
    if (!Stream->ReaderFFSContext)
    {
        FMContext Tmp = create_local_FMcontext();
        Stream->ReaderFFSContext = create_FFSContext_FM(Tmp);
        free_FMcontext(Tmp);
    }

    LoadFormats(Stream, MetaData->Formats);

    LoadAttributes(Stream, MetaData);
}

extern void FFSMarshalInstallMetadata(SstStream Stream, TSMetadataMsg MetaData)
{
    FFSMarshalInstallPreciousMetadata(Stream, MetaData);

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

static int FFSBitfieldTest(struct FFSMetadataInfoStruct *MBase, int Bit)
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
    return ((MBase->BitField[Element] & (1 << ElementBit)) ==
            (1 << ElementBit));
}

extern void SstFFSSetZFPParams(SstStream Stream, attr_list Attrs)
{
    if (Stream->WriterMarshalData)
    {
        struct FFSWriterMarshalBase *Info = Stream->WriterMarshalData;
        if (Info->ZFPParams)
            free_attr_list(Info->ZFPParams);
        add_ref_attr_list(Attrs);
        Info->ZFPParams = Attrs;
    }
}

extern void SstFFSMarshal(SstStream Stream, void *Variable, const char *Name,
                          const char *Type, size_t ElemSize, size_t DimCount,
                          const size_t *Shape, const size_t *Count,
                          const size_t *Offsets, const void *Data)
{

    struct FFSWriterMarshalBase *Info;
    struct FFSMetadataInfoStruct *MBase;

    FFSWriterRec Rec = LookupWriterRec(Stream, Variable);
    if (!Rec)
    {
        Rec = CreateWriterRec(Stream, Variable, Name, Type, ElemSize, DimCount);
    }
    Info = (struct FFSWriterMarshalBase *)Stream->WriterMarshalData;

    MBase = Stream->M;
    FFSBitfieldSet(MBase, Rec->FieldID);

    if (Rec->DimCount == 0)
    {
        char *base = Stream->M + Rec->MetaOffset;
        memcpy(base, Data, ElemSize);
    }
    else
    {
        MetaArrayRec *MetaEntry = Stream->M + Rec->MetaOffset;
        ArrayRec *DataEntry = Stream->D + Rec->DataOffset;

        /* handle metadata */
        MetaEntry->Dims = DimCount;
        if (Shape)
            MetaEntry->Shape = CopyDims(DimCount, Shape);
        else
            MetaEntry->Shape = NULL;
        MetaEntry->Count = CopyDims(DimCount, Count);
        if (Offsets)
            MetaEntry->Offsets = CopyDims(DimCount, Offsets);
        else
            MetaEntry->Offsets = NULL;

        if ((Stream->ConfigParams->CompressionMethod == SstCompressZFP) &&
            ZFPcompressionPossible(Type, DimCount))
        {
#ifdef ADIOS2_HAVE_ZFP
            /* this should never be true if ZFP is not available */
            size_t ByteCount;
            char *Output = FFS_ZFPCompress(Stream, Rec->DimCount, Rec->Type,
                                           (void *)Data, Count, &ByteCount);
            DataEntry->ElemCount = ByteCount;
            DataEntry->Array = Output;
#endif
        }
        else
        {
            /* normal array case */
            size_t ElemCount = CalcSize(DimCount, Count);
            DataEntry->ElemCount = ElemCount;
            /* this is PutSync case, so we have to copy the data NOW */
            DataEntry->Array = malloc(ElemCount * ElemSize);
            memcpy(DataEntry->Array, Data, ElemCount * ElemSize);
        }
    }
}

extern void SstFFSMarshalAttribute(SstStream Stream, const char *Name,
                                   const char *Type, size_t ElemSize,
                                   size_t ElemCount, const void *Data)
{

    struct FFSWriterMarshalBase *Info;
    Info = (struct FFSWriterMarshalBase *)Stream->WriterMarshalData;
    const char *String = NULL;
    const char *DataAddress = Data;

    if (strcmp(Type, "string") == 0)
    {
        ElemSize = sizeof(char *);
        String = Data;
        DataAddress = (const char *)&String;
    }
    if (ElemCount == -1)
    {
        // simple field, only simple attribute name and value
        char *SstName = BuildVarName(Name, Type, ElemSize);
        AddField(&Info->AttributeFields, &Info->AttributeFieldCount, SstName,
                 Type, ElemSize);
        free(SstName);
        RecalcAttributeStorageSize(Stream);
        int DataOffset =
            Info->AttributeFields[Info->AttributeFieldCount - 1].field_offset;
        memcpy(Info->AttributeData + DataOffset, DataAddress, ElemSize);
    }
    else
    {
        /* // Array field.  To Metadata, add FMFields for DimCount, Shape, Count
         */
        /* // and Offsets matching _MetaArrayRec */
        /* char *ArrayName = BuildStaticArrayName(Name, Type, ElemCount); */
        /* AddField(&Info->AttributeFields, &Info->AttributeFieldCount,
         * ArrayName, Type, */
        /*          sizeof(size_t)); */
        /* free(ArrayName); */
        /* Rec->MetaOffset = */
        /*     Info->MetaFields[Info->MetaFieldCount - 1].field_offset; */
        /* char *ShapeName = ConcatName(Name, "Shape"); */
        /* char *CountName = ConcatName(Name, "Count"); */
        /* char *OffsetsName = ConcatName(Name, "Offsets"); */
        /* AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount,
         * ShapeName, */
        /*                    "integer", sizeof(size_t), DimCount); */
        /* AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount,
         * CountName, */
        /*                    "integer", sizeof(size_t), DimCount); */
        /* AddFixedArrayField(&Info->MetaFields, &Info->MetaFieldCount, */
        /*                    OffsetsName, "integer", sizeof(size_t), DimCount);
         */
        /* free(ShapeName); */
        /* free(CountName); */
        /* free(OffsetsName); */
        /* RecalcMarshalStorageSize(Stream); */

        /* if ((Stream->ConfigParams->CompressionMethod == SstCompressZFP) && */
        /*     ZFPcompressionPossible(Type, DimCount)) */
        /* { */
        /*     Type = "char"; */
        /*     ElemSize = 1; */
        /* } */
        /* // To Data, add FMFields for ElemCount and Array matching _ArrayRec
         */
        /* char *ElemCountName = ConcatName(Name, "ElemCount"); */
        /* AddField(&Info->DataFields, &Info->DataFieldCount, ElemCountName, */
        /*          "integer", sizeof(size_t)); */
        /* Rec->DataOffset = */
        /*     Info->DataFields[Info->DataFieldCount - 1].field_offset; */
        /* char *SstName = ConcatName(Name, ""); */
        /* AddVarArrayField(&Info->DataFields, &Info->DataFieldCount, SstName,
         */
        /*                  Type, ElemSize, ElemCountName); */
        /* free(SstName); */
        /* free(ElemCountName); */
        /* RecalcMarshalStorageSize(Stream); */
        /* // Changing the formats renders these invalid */
        /* Info->MetaFormat = NULL; */
        /* Info->DataFormat = NULL; */
    }
}
