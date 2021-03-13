/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Serializer.h
 *
 */

#include "adios2/core/Attribute.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"

#include "BP5Deserializer.h"

#include <string.h>

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace adios2
{
namespace format
{
void BP5Deserializer::InstallMetaMetaData(MetaMetaInfoBlock &MM)
{
    char *FormatID = (char *)malloc(MM.MetaMetaIDLen);
    char *MetaMetaInfo = (char *)malloc(MM.MetaMetaInfoLen);
    memcpy(FormatID, MM.MetaMetaID, MM.MetaMetaIDLen);
    memcpy(MetaMetaInfo, MM.MetaMetaInfo, MM.MetaMetaInfoLen);
    load_external_format_FMcontext(FMContext_from_FFS(ReaderFFSContext),
                                   FormatID, MM.MetaMetaIDLen, MetaMetaInfo);
    free(FormatID);
}

BP5Deserializer::ControlInfo *BP5Deserializer::GetPriorControl(FMFormat Format)
{
    struct ControlInfo *tmp = ControlBlocks;
    while (tmp)
    {
        if (tmp->Format == Format)
        {
            return tmp;
        }
        tmp = tmp->Next;
    }
    return NULL;
}

bool BP5Deserializer::NameIndicatesArray(const char *Name)
{
    int Len = strlen(Name);
    return (strcmp("Dims", Name + Len - 4) == 0);
}

DataType BP5Deserializer::TranslateFFSType2ADIOS(const char *Type, int size)
{
    if (strcmp(Type, "integer") == 0)
    {
        if (size == 1)
        {
            return DataType::Int8;
        }
        else if (size == 2)
        {
            return DataType::Int16;
        }
        else if (size == 4)
        {
            return DataType::Int32;
        }
        else if (size == 8)
        {
            return DataType::Int64;
        }
    }
    else if (strcmp(Type, "unsigned integer") == 0)
    {
        if (size == 1)
        {
            return DataType::UInt8;
        }
        else if (size == 2)
        {
            return DataType::UInt16;
        }
        else if (size == 4)
        {
            return DataType::UInt32;
        }
        else if (size == 8)
        {
            return DataType::UInt64;
        }
    }
    else if ((strcmp(Type, "double") == 0) || (strcmp(Type, "float") == 0))
    {
        if (size == sizeof(float))
        {
            return DataType::Float;
        }
        else if ((sizeof(long double) != sizeof(double)) &&
                 (size == sizeof(long double)))
        {
            return DataType::Double;
        }
        else
        {
            return DataType::Double;
        }
    }
    else if (strcmp(Type, "complex4") == 0)
    {
        return DataType::FloatComplex;
    }
    else if (strcmp(Type, "complex8") == 0)
    {
        return DataType::DoubleComplex;
    }
    return DataType::None;
}

void BP5Deserializer::BreakdownArrayName(const char *Name, char **base_name_p,
                                         DataType *type_p, int *element_size_p)
{
    int Type;
    int ElementSize;
    const char *NameStart = strchr(strchr(Name, '_') + 1, '_') + 1;
    sscanf(Name, "SST%d_%d_", &ElementSize, &Type);
    *element_size_p = ElementSize;
    *type_p = (DataType)Type;
    *base_name_p = strdup(NameStart);
    (*base_name_p)[strlen(*base_name_p) - 4] = 0; // kill "Dims"
}

BP5Deserializer::FFSVarRec *BP5Deserializer::LookupVarByKey(void *Key)
{
    return NULL;
}

BP5Deserializer::FFSVarRec *BP5Deserializer::LookupVarByName(const char *Name)
{
    auto ret = VarByName[Name];
    std::cout << "Lookup var by name for name " << Name << " returning " << ret
              << std::endl;

    return ret;
}

BP5Deserializer::FFSVarRec *BP5Deserializer::CreateVarRec(const char *ArrayName)
{
    FFSVarRec *Ret = new FFSVarRec(WriterCohortSize);
    Ret->VarName = strdup(ArrayName);
    Ret->Variable = nullptr;
    VarByName[Ret->VarName] = Ret;
    std::cout << "Creating variable Rec with name " << ArrayName << std::endl;

    return Ret;
}

BP5Deserializer::ControlInfo *BP5Deserializer::BuildControl(FMFormat Format)
{
    FMStructDescList FormatList = format_list_of_FMFormat(Format);
    FMFieldList FieldList = FormatList[0].field_list;
    while (strncmp(FieldList->field_name, "BitField", 8) == 0)
        FieldList++;
    while (FieldList->field_name &&
           (strncmp(FieldList->field_name, "DataBlockSize", 8) == 0))
        FieldList++;
    int i = 0;
    int ControlCount = 0;
    ControlInfo *ret = (BP5Deserializer::ControlInfo *)malloc(sizeof(*ret));
    ret->Format = Format;
    while (FieldList[i].field_name)
    {
        ret = (ControlInfo *)realloc(
            ret, sizeof(*ret) + ControlCount * sizeof(struct ControlInfo));
        struct ControlStruct *C = &(ret->Controls[ControlCount]);
        ControlCount++;

        C->FieldIndex = i;
        C->FieldOffset = FieldList[i].field_offset;

        printf("Working on Field %s\n", FieldList[i].field_name);
        if (NameIndicatesArray(FieldList[i].field_name))
        {
            char *ArrayName;
            DataType Type;
            FFSVarRec *VarRec = nullptr;
            int ElementSize;
            C->IsArray = 1;
            printf("Working on Array Field %s\n", FieldList[i].field_name);
            BreakdownArrayName(FieldList[i].field_name, &ArrayName, &Type,
                               &ElementSize);
            //            if (WriterRank != 0)
            //            {
            VarRec = LookupVarByName(ArrayName);
            //            }
            if (!VarRec)
            {
                VarRec = CreateVarRec(ArrayName);
                VarRec->Type = Type;
                VarRec->ElementSize = ElementSize;
                C->ElementSize = ElementSize;
            }
            i += 7; // number of fields in MetaArrayRec
            free(ArrayName);
            C->VarRec = VarRec;
        }
        else
        {
            /* simple field */
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip SST_
            FFSVarRec *VarRec = NULL;
            C->IsArray = 0;
            printf("Working on Simple Field %s\n", FieldList[i].field_name);
            VarRec = LookupVarByName(FieldName);
            if (!VarRec)
            {
                DataType Type = TranslateFFSType2ADIOS(FieldList[i].field_type,
                                                       FieldList[i].field_size);
                VarRec = CreateVarRec(FieldName);
                VarRec->DimCount = 0;
                C->Type = Type;
                VarRec->Type = Type;
            }
            VarRec->ElementSize = FieldList[i].field_size;
            C->ElementSize = FieldList[i].field_size;
            C->VarRec = VarRec;
            free(FieldName);
            i++;
        }
    }
    ret->ControlCount = ControlCount;
    ret->Next = ControlBlocks;
    ControlBlocks = ret;
    return ret;
}

void BP5Deserializer::ReverseDimensions(size_t *Dimensions, int count)
{
    for (int i = 0; i < count / 2; i++)
    {
        size_t tmp = Dimensions[i];
        Dimensions[i] = Dimensions[count - i - 1];
        Dimensions[count - i - 1] = tmp;
    }
}

void *BP5Deserializer::VarSetup(core::Engine *engine, const char *variableName,
                                const DataType Type, void *data)
{
    std::cout << "Setting up variable with name " << variableName
              << " and type " << Type << std::endl;
    if (Type == adios2::DataType::Compound)
    {
        return (void *)NULL;
    }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> *variable =                                          \
            &(engine->m_IO.DefineVariable<T>(variableName));                   \
        std::cout << "Really " << variableName << " and type " << Type         \
                  << std::endl;                                                \
        variable->SetData((T *)data);                                          \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    return (void *)NULL;
};

void *BP5Deserializer::ArrayVarSetup(core::Engine *engine,
                                     const char *variableName,
                                     const DataType type, int DimCount,
                                     size_t *Shape, size_t *Start,
                                     size_t *Count)
{
    std::vector<size_t> VecShape;
    std::vector<size_t> VecStart;
    std::vector<size_t> VecCount;
    adios2::DataType Type = (adios2::DataType)type;
    /*
     * setup shape of array variable as global (I.E. Count == Shape,
     * Start == 0)
     */
    std::cout << "Setting up array variable with name " << variableName
              << std::endl;
    if (Shape)
    {
        for (int i = 0; i < DimCount; i++)
        {
            VecShape.push_back(Shape[i]);
            VecStart.push_back(0);
            VecCount.push_back(Shape[i]);
        }
    }
    else
    {
        VecShape = {};
        VecStart = {};
        for (int i = 0; i < DimCount; i++)
        {
            VecCount.push_back(Count[i]);
        }
    }

    if (Type == adios2::DataType::Compound)
    {
        return (void *)NULL;
    }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> *variable = &(engine->m_IO.DefineVariable<T>(        \
            variableName, VecShape, VecStart, VecCount));                      \
        variable->m_AvailableStepsCount = 1;                                   \
        return (void *)variable;                                               \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    return (void *)NULL;
};

void BP5Deserializer::InstallMetaData(void *MetadataBlock, size_t BlockLen,
                                      size_t WriterRank)
{
    FFSTypeHandle FFSformat;
    void *BaseData;
    static int DumpMetadata = -1;
    FFSformat =
        FFSTypeHandle_from_encode(ReaderFFSContext, (char *)MetadataBlock);
    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(ReaderFFSContext);
        FMFormat Format = FMformat_from_ID(FMC, (char *)MetadataBlock);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        FMlocalize_structs(List);
        establish_conversion(ReaderFFSContext, FFSformat, List);
        FMfree_struct_list(List);
    }
    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(ReaderFFSContext, (char *)MetadataBlock, &BaseData);
    }
    else
    {
        int DecodedLength = FFS_est_decode_length(
            ReaderFFSContext, (char *)MetadataBlock, BlockLen);
        BaseData = malloc(DecodedLength);
        FFSdecode_to_buffer(ReaderFFSContext, (char *)MetadataBlock, BaseData);
    }
    if (DumpMetadata == -1)
    {
        DumpMetadata = (getenv("BP5DumpMetadata") != NULL);
    }
    if (DumpMetadata && (WriterRank == 0))
    {
        printf("\nIncomingMetadatablock from WriterRank %d is %p :\n",
               (int)WriterRank, BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
        printf("\n\n");
    }
    struct ControlInfo *Control;
    struct ControlStruct *ControlArray;
    Control = GetPriorControl(FMFormat_of_original(FFSformat));
    if (!Control)
    {
        Control = BuildControl(FMFormat_of_original(FFSformat));
    }
    ControlArray = &Control->Controls[0];

    MetadataBaseAddrs[WriterRank] = BaseData;
    for (int i = 0; i < Control->ControlCount; i++)
    {
        int FieldOffset = ControlArray[i].FieldOffset;
        FFSVarRec *VarRec = ControlArray[i].VarRec;
        void *field_data = (char *)BaseData + FieldOffset;
        if (!FFSBitfieldTest((FFSMetadataInfoStruct *)BaseData, i))
        {
            continue;
        }
        if (ControlArray[i].IsArray)
        {
            MetaArrayRec *meta_base = (MetaArrayRec *)field_data;
            if ((meta_base->Dims > 1) && (WriterIsRowMajor != ReaderIsRowMajor))
            {
                /* if we're getting data from someone of the other array gender,
                 * switcheroo */
                ReverseDimensions(meta_base->Shape, meta_base->Dims);
                ReverseDimensions(meta_base->Count, meta_base->Dims);
                ReverseDimensions(meta_base->Offsets, meta_base->Dims);
            }
            if (WriterRank == 0)
            {
                VarRec->GlobalDims = meta_base->Shape;
            }
            if (!VarRec->Variable)
            {
                VarRec->Variable = ArrayVarSetup(
                    m_Engine, VarRec->VarName, VarRec->Type, meta_base->Dims,
                    meta_base->Shape, meta_base->Offsets, meta_base->Count);
            }
            VarRec->DimCount = meta_base->Dims;
            VarRec->PerWriterBlockCount[WriterRank] =
                meta_base->Dims ? meta_base->DBCount / meta_base->Dims : 1;
            VarRec->PerWriterStart[WriterRank] = meta_base->Offsets;
            VarRec->PerWriterCounts[WriterRank] = meta_base->Count;
            if (WriterRank == 0)
            {
                VarRec->PerWriterBlockStart[WriterRank] = 0;
            }
            if (WriterRank < WriterCohortSize - 1)
            {
                VarRec->PerWriterBlockStart[WriterRank + 1] =
                    VarRec->PerWriterBlockStart[WriterRank] +
                    VarRec->PerWriterBlockCount[WriterRank];
            }
#ifdef NOTDEF
            // needs to be replaced with Simple Blocks Info
            for (int i = 0; i < VarRec->PerWriterBlockCount[WriterRank]; i++)
            {
                size_t *Offsets = NULL;
                if (meta_base->Offsets)
                    Offsets = meta_base->Offsets + (i * meta_base->Dims);
                ArrayBlocksInfoUpcall(m_Engine, VarRec->Variable, VarRec->Type,
                                      WriterRank, meta_base->Dims,
                                      meta_base->Shape, Offsets,
                                      meta_base->Count);
            }
#endif
        }
        else
        {
            if (!VarRec->Variable)
            {
                VarRec->Variable = VarSetup(m_Engine, VarRec->VarName,
                                            VarRec->Type, field_data);
            }
            VarRec->PerWriterMetaFieldOffset[WriterRank] = FieldOffset;
        }
    }
}

BP5Deserializer::BP5Deserializer(int WriterCount)
{
    FMContext Tmp = create_local_FMcontext();
    ReaderFFSContext = create_FFSContext_FM(Tmp);
    free_FMcontext(Tmp);
    WriterCohortSize = WriterCount;
    //    WriterInfo.resize(WriterCohortSize);
    MetadataBaseAddrs.resize(WriterCohortSize);
    MetadataFieldLists.resize(WriterCohortSize);
    DataBaseAddrs.resize(WriterCohortSize);
}

BP5Deserializer::~BP5Deserializer() { free_FFSContext(ReaderFFSContext); }

}
}
