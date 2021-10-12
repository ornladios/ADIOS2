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
#include "adios2/core/VariableBase.h"

#include "BP5Deserializer.h"
#include "BP5Deserializer.tcc"

#include <float.h>
#include <limits.h>
#include <math.h>
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

bool BP5Deserializer::NameIndicatesAttrArray(const char *Name)
{
    int Len = strlen(Name);
    return (strcmp("ElemCount", Name + Len - 9) == 0);
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
            return DataType::LongDouble;
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
    else if (strcmp(Type, "string") == 0)
    {
        return DataType::String;
    }

    return DataType::None;
}

void BP5Deserializer::BreakdownVarName(const char *Name, char **base_name_p,
                                       DataType *type_p, int *element_size_p)
{
    int Type;
    int ElementSize;
    const char *NameStart = strchr(strchr(Name, '_') + 1, '_') + 1;
    // + 3 to skip BP5 or bp5 prefix
    sscanf(Name + 3, "%d_%d_", &ElementSize, &Type);
    *element_size_p = ElementSize;
    *type_p = (DataType)Type;
    *base_name_p = strdup(NameStart);
}

void BP5Deserializer::BreakdownArrayName(const char *Name, char **base_name_p,
                                         DataType *type_p, int *element_size_p)
{
    int Type;
    int ElementSize;
    const char *NameStart = strchr(strchr(Name, '_') + 1, '_') + 1;
    // + 3 to skip BP5 or bp5 prefix
    sscanf(Name + 3, "%d_%d_", &ElementSize, &Type);
    *element_size_p = ElementSize;
    *type_p = (DataType)Type;
    *base_name_p = strdup(NameStart);
    (*base_name_p)[strlen(*base_name_p) - 4] = 0; // kill "Dims"
}

BP5Deserializer::BP5VarRec *BP5Deserializer::LookupVarByKey(void *Key)
{
    auto ret = VarByKey[Key];
    return ret;
}

BP5Deserializer::BP5VarRec *BP5Deserializer::LookupVarByName(const char *Name)
{
    auto ret = VarByName[Name];
    return ret;
}

BP5Deserializer::BP5VarRec *BP5Deserializer::CreateVarRec(const char *ArrayName)
{
    BP5VarRec *Ret = new BP5VarRec();
    Ret->VarName = strdup(ArrayName);
    Ret->Variable = nullptr;
    Ret->VarNum = m_VarCount++;
    VarByName[Ret->VarName] = Ret;
    if (!m_RandomAccessMode)
    {
        Ret->PerWriterMetaFieldOffset.resize(m_WriterCohortSize);
        Ret->PerWriterBlockStart.resize(m_WriterCohortSize);
    }
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
    ret->MetaFieldOffset = new std::vector<size_t>();
    ret->CIVarIndex = new std::vector<size_t>();
    size_t VarIndex = 0;
    while (FieldList[i].field_name)
    {
        ret = (ControlInfo *)realloc(
            ret, sizeof(*ret) + ControlCount * sizeof(struct ControlInfo));
        struct ControlStruct *C = &(ret->Controls[ControlCount]);
        ControlCount++;

        C->FieldOffset = FieldList[i].field_offset;
        C->OrigShapeID = ShapeID::Unknown;
        switch (FieldList[i].field_name[2])
        {
        case 'g':
            C->OrigShapeID = ShapeID::GlobalValue;
            break;
        case 'G':
            C->OrigShapeID = ShapeID::GlobalArray;
            break;
        case 'J':
            C->OrigShapeID = ShapeID::JoinedArray;
            break;
        case 'l':
            C->OrigShapeID = ShapeID::LocalValue;
            break;
        case 'L':
            C->OrigShapeID = ShapeID::LocalArray;
            break;
        }
        if (strncmp(FieldList[i].field_name, "SST", 3) == 0)
        {
            if (NameIndicatesArray(FieldList[i].field_name))
            {
                C->OrigShapeID = ShapeID::LocalArray;
            }
            else
            {
                C->OrigShapeID = ShapeID::GlobalValue;
            }
        }
        BP5VarRec *VarRec = nullptr;
        if (NameIndicatesArray(FieldList[i].field_name))
        {
            char *ArrayName;
            DataType Type;
            int ElementSize;
            BreakdownArrayName(FieldList[i].field_name, &ArrayName, &Type,
                               &ElementSize);
            VarRec = LookupVarByName(ArrayName);
            if (!VarRec)
            {
                VarRec = CreateVarRec(ArrayName);
                VarRec->Type = Type;
                VarRec->ElementSize = ElementSize;
                VarRec->OrigShapeID = C->OrigShapeID;
                C->ElementSize = ElementSize;
            }
            i += 7; // number of fields in MetaArrayRec
            free(ArrayName);
            C->VarRec = VarRec;
        }
        else
        {
            /* simple field */
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip BP5_
            VarRec = LookupVarByName(FieldName);
            if (!VarRec)
            {
                DataType Type = TranslateFFSType2ADIOS(FieldList[i].field_type,
                                                       FieldList[i].field_size);
                VarRec = CreateVarRec(FieldName);
                VarRec->DimCount = 0;
                C->Type = Type;
                VarRec->OrigShapeID = C->OrigShapeID;
                VarRec->Type = Type;
            }
            VarRec->ElementSize = FieldList[i].field_size;
            C->ElementSize = FieldList[i].field_size;
            C->VarRec = VarRec;
            free(FieldName);
            i++;
        }
        if (ret->MetaFieldOffset->size() <= VarRec->VarNum)
        {
            ret->MetaFieldOffset->resize(VarRec->VarNum + 1);
            ret->CIVarIndex->resize(VarRec->VarNum + 1);
        }
        (*ret->CIVarIndex)[VarRec->VarNum] = VarIndex;
        (*ret->MetaFieldOffset)[VarRec->VarNum] = C->FieldOffset;
        VarIndex++;
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
    if (Type == adios2::DataType::Compound)
    {
        return (void *)NULL;
    }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Variable<T> *variable =                                          \
            &(engine->m_IO.DefineVariable<T>(variableName));                   \
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
        core::Variable<T> *variable =                                          \
            &(engine->m_IO.DefineVariable<T>(variableName));                   \
        variable->m_Shape = VecShape;                                          \
        variable->m_Start = VecStart;                                          \
        variable->m_Count = VecCount;                                          \
        variable->m_AvailableStepsCount = 1;                                   \
        variable->m_ShapeID = ShapeID::GlobalArray;                            \
        variable->m_SingleValue = false;                                       \
        return (void *)variable;                                               \
    }
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    return (void *)NULL;
};

void BP5Deserializer::SetupForTimestep(size_t Timestep)
{
    CurTimestep = Timestep;
    PendingRequests.clear();

    for (auto RecPair : VarByKey)
    {
        m_Engine->m_IO.RemoveVariable(RecPair.second->VarName);
        RecPair.second->Variable = NULL;
    }
}

void BP5Deserializer::InstallMetaData(void *MetadataBlock, size_t BlockLen,
                                      size_t WriterRank, size_t Step)
{
    FFSTypeHandle FFSformat;
    void *BaseData;
    static int DumpMetadata = -1;
    FFSformat =
        FFSTypeHandle_from_encode(ReaderFFSContext, (char *)MetadataBlock);
    if (!FFSformat)
    {
        throw std::logic_error("Internal error or file corruption, no know "
                               "format for Metadata Block");
    }
    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(ReaderFFSContext);
        FMFormat Format = FMformat_from_ID(FMC, (char *)MetadataBlock);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        // GSE - restrict to homogenous FTM       FMlocalize_structs(List);
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
    if (DumpMetadata)
    {
        printf("\nIncomingMetadatablock from WriterRank %d is %p :\n",
               (int)WriterRank, BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
        printf("\n\n");
    }
    struct ControlInfo *Control;
    struct ControlStruct *ControlFields;
    Control = GetPriorControl(FMFormat_of_original(FFSformat));
    if (!Control)
    {
        Control = BuildControl(FMFormat_of_original(FFSformat));
    }
    ControlFields = &Control->Controls[0];

    if (m_RandomAccessMode)
    {
        if (m_ControlArray.size() < Step + 1)
        {
            m_ControlArray.resize(Step + 1);
        }
        if (m_ControlArray[Step].size() == 0)
        {
            m_ControlArray[Step].resize(m_WriterCohortSize);
        }
        m_ControlArray[Step][WriterRank] = Control;

        MetadataBaseArray.resize(Step + 1);
        if (MetadataBaseArray[Step] == nullptr)
        {
            m_MetadataBaseAddrs = new std::vector<void *>();
            m_MetadataBaseAddrs->resize(m_WriterCohortSize);
            MetadataBaseArray[Step] = m_MetadataBaseAddrs;
            m_FreeableMBA = nullptr;
        }
    }
    else
    {
        if (!m_MetadataBaseAddrs)
        {
            m_MetadataBaseAddrs = new std::vector<void *>();
            m_FreeableMBA = m_MetadataBaseAddrs;
            m_MetadataBaseAddrs->resize(m_WriterCohortSize);
        }
    }
    (*m_MetadataBaseAddrs)[WriterRank] = BaseData;

    for (int i = 0; i < Control->ControlCount; i++)
    {
        size_t FieldOffset = ControlFields[i].FieldOffset;
        BP5VarRec *VarRec = ControlFields[i].VarRec;
        void *field_data = (char *)BaseData + FieldOffset;
        if (!BP5BitfieldTest((BP5MetadataInfoStruct *)BaseData, i))
        {
            continue;
        }
        if (!m_RandomAccessMode)
            VarRec->PerWriterMetaFieldOffset[WriterRank] = FieldOffset;
        if ((ControlFields[i].OrigShapeID == ShapeID::GlobalArray) ||
            (ControlFields[i].OrigShapeID == ShapeID::LocalArray))
        {
            MetaArrayRec *meta_base = (MetaArrayRec *)field_data;
            if ((meta_base->Dims > 1) &&
                (m_WriterIsRowMajor != m_ReaderIsRowMajor))
            {
                /* if we're getting data from someone of the other array gender,
                 * switcheroo */
                ReverseDimensions(meta_base->Shape, meta_base->Dims);
                ReverseDimensions(meta_base->Count, meta_base->Dims);
                ReverseDimensions(meta_base->Offsets, meta_base->Dims);
            }
            if ((WriterRank == 0) || (VarRec->GlobalDims == NULL))
            {
                // use the shape from rank 0 (or first non-NULL)
                VarRec->GlobalDims = meta_base->Shape;
            }
            if (!VarRec->Variable)
            {
                VarRec->Variable = ArrayVarSetup(
                    m_Engine, VarRec->VarName, VarRec->Type, meta_base->Dims,
                    meta_base->Shape, meta_base->Offsets, meta_base->Count);
                static_cast<VariableBase *>(VarRec->Variable)->m_Engine =
                    m_Engine;
                VarByKey[VarRec->Variable] = VarRec;
                VarRec->LastTSAdded = Step; // starts at 1
                if (!meta_base->Shape)
                {
                    static_cast<VariableBase *>(VarRec->Variable)->m_ShapeID =
                        ShapeID::LocalArray;
                }
            }

            VarRec->DimCount = meta_base->Dims;
            size_t BlockCount =
                meta_base->Dims ? meta_base->DBCount / meta_base->Dims : 1;
            if (!m_RandomAccessMode)
            {
                if (WriterRank == 0)
                {
                    VarRec->PerWriterBlockStart[WriterRank] = 0;
                    if (m_WriterCohortSize > 1)
                        VarRec->PerWriterBlockStart[WriterRank + 1] =
                            BlockCount;
                }
                if (WriterRank < static_cast<size_t>(m_WriterCohortSize - 1))
                {
                    VarRec->PerWriterBlockStart[WriterRank + 1] =
                        VarRec->PerWriterBlockStart[WriterRank] + BlockCount;
                }
            }
            else
            {
                //   Random access, add to m_AvailableShapes
                if ((VarRec->LastShapeAdded != Step) && meta_base->Shape)
                {
                    std::vector<size_t> shape;
                    for (size_t i = 0; i < meta_base->Dims; i++)
                    {
                        shape.push_back(meta_base->Shape[i]);
                    }
                    static_cast<VariableBase *>(VarRec->Variable)
                        ->m_AvailableShapes[Step + 1] = shape;
                    VarRec->LastShapeAdded = Step;
                }
            }
        }
        else
        {
            if (!VarRec->Variable)
            {
                if (ControlFields[i].OrigShapeID == ShapeID::LocalValue)
                {
                    // Local single values show up as global arrays on the
                    // reader
                    size_t zero = 0;
                    size_t writerSize = m_WriterCohortSize;
                    VarRec->Variable =
                        ArrayVarSetup(m_Engine, VarRec->VarName, VarRec->Type,
                                      1, &writerSize, &zero, &writerSize);
                    auto VB = static_cast<VariableBase *>(VarRec->Variable);
                    static_cast<VariableBase *>(VarRec->Variable)->m_Engine =
                        m_Engine;
                    VB->m_ShapeID = ShapeID::GlobalArray;
                }
                else
                {
                    // Global single value
                    VarRec->Variable = VarSetup(m_Engine, VarRec->VarName,
                                                VarRec->Type, field_data);
                    static_cast<VariableBase *>(VarRec->Variable)->m_Engine =
                        m_Engine;
                }
                VarByKey[VarRec->Variable] = VarRec;
                VarRec->LastTSAdded = Step; // starts at 1
            }
        }
        if (m_RandomAccessMode && (VarRec->LastTSAdded != Step))
        {
            static_cast<VariableBase *>(VarRec->Variable)
                ->m_AvailableStepsCount++;
            VarRec->LastTSAdded = Step;
        }
        if (VarRec->FirstTSSeen == SIZE_MAX)
        {
            VarRec->FirstTSSeen = Step;
        }
    }
}

void BP5Deserializer::InstallAttributeData(void *AttributeBlock,
                                           size_t BlockLen, size_t Step)
{
    static int DumpMetadata = -1;
    FMFieldList FieldList;
    FMStructDescList FormatList;
    void *BaseData;
    FFSTypeHandle FFSformat;

    if (BlockLen == 0)
        return;

    m_Engine->m_IO.RemoveAllAttributes();
    FFSformat =
        FFSTypeHandle_from_encode(ReaderFFSContext, (char *)AttributeBlock);
    if (!FFSformat)
    {
        throw std::logic_error("Internal error or file corruption, no know "
                               "format for Attribute Block");
    }
    if (!FFShas_conversion(FFSformat))
    {
        FMContext FMC = FMContext_from_FFS(ReaderFFSContext);
        FMFormat Format = FMformat_from_ID(FMC, (char *)AttributeBlock);
        FMStructDescList List =
            FMcopy_struct_list(format_list_of_FMFormat(Format));
        // GSE - restrict to homogenous FTM       FMlocalize_structs(List);
        establish_conversion(ReaderFFSContext, FFSformat, List);
        FMfree_struct_list(List);
    }

    if (FFSdecode_in_place_possible(FFSformat))
    {
        FFSdecode_in_place(ReaderFFSContext, (char *)AttributeBlock, &BaseData);
    }
    else
    {
        int DecodedLength = FFS_est_decode_length(
            ReaderFFSContext, (char *)AttributeBlock, BlockLen);
        BaseData = malloc(DecodedLength);
        FFSBuffer decode_buf =
            create_fixed_FFSBuffer((char *)BaseData, DecodedLength);
        FFSdecode_to_buffer(ReaderFFSContext, (char *)AttributeBlock,
                            decode_buf);
    }
    if (DumpMetadata == -1)
    {
        DumpMetadata = (getenv("BP5DumpMetadata") != NULL);
    }
    if (DumpMetadata)
    {
        printf("\nIncomingAttributeDatablock (Step %zu) is %p :\n", Step,
               BaseData);
        FMdump_data(FMFormat_of_original(FFSformat), BaseData, 1024000);
        printf("\n\n");
    }
    FormatList = format_list_of_FMFormat(FMFormat_of_original(FFSformat));
    FieldList = FormatList[0].field_list;
    int i = 0;
    while (FieldList[i].field_name)
    {
        char *FieldName;
        void *field_data = (char *)BaseData + FieldList[i].field_offset;

        if (!NameIndicatesAttrArray(FieldList[i].field_name))
        {
            DataType Type;
            int ElemSize;
            BreakdownVarName(FieldList[i].field_name, &FieldName, &Type,
                             &ElemSize);
            if (Type == adios2::DataType::Compound)
            {
                return;
            }
            else if (Type == helper::GetDataType<std::string>())
            {
                m_Engine->m_IO.DefineAttribute<std::string>(
                    FieldName, *(char **)field_data);
            }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        m_Engine->m_IO.DefineAttribute<T>(FieldName, *(T *)field_data);        \
    }

            ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
            else
            {
                std::cout << "Loading attribute matched no type "
                          << ToString(Type) << std::endl;
            }
            free(FieldName);
            i++;
        }
        else
        {
            DataType Type;
            size_t ElemCount = *(size_t *)field_data;
            field_data = (void *)((char *)field_data + sizeof(size_t));
            i++;
            char *FieldName = strdup(FieldList[i].field_name + 4); // skip BP5_
            char *FieldType = strdup(FieldList[i].field_type);
            *index(FieldType, '[') = 0;
            Type = (DataType)TranslateFFSType2ADIOS(FieldType,
                                                    FieldList[i].field_size);
            if (Type == adios2::DataType::Compound)
            {
                return;
            }
            else if (Type == helper::GetDataType<std::string>())
            {
                std::vector<std::string> array;
                array.resize(ElemCount);
                char **str_array = *(char ***)field_data;
                for (size_t i = 0; i < ElemCount; i++)
                {
                    array[i].assign(str_array[i]);
                }
                m_Engine->m_IO.DefineAttribute<std::string>(
                    FieldName, array.data(), array.size());
            }
#define declare_type(T)                                                        \
    else if (Type == helper::GetDataType<T>())                                 \
    {                                                                          \
        T **array = *(T ***)field_data;                                        \
        m_Engine->m_IO.DefineAttribute<T>(FieldName, (T *)array, ElemCount);   \
    }

            ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
            else
            {
                std::cout << "Loading attribute matched no type "
                          << ToString(Type) << std::endl;
            }
            free(FieldName);
            i++;
        }
    }
}

bool BP5Deserializer::QueueGet(core::VariableBase &variable, void *DestData)
{
    if (!m_RandomAccessMode)
    {
        return QueueGetSingle(variable, DestData, CurTimestep);
    }
    else
    {
        bool ret = false;
        if (variable.m_StepsStart + variable.m_StepsCount >
            variable.m_AvailableStepsCount)
        {
            throw std::invalid_argument(
                "ERROR: offset " + std::to_string(variable.m_StepsCount) +
                " from steps start " + std::to_string(variable.m_StepsStart) +
                " in variable " + variable.m_Name +
                " is beyond the largest available step = " +
                std::to_string(variable.m_AvailableStepsCount +
                               variable.m_AvailableStepsStart) +
                ", check Variable SetStepSelection argument stepsCount "
                "(random access), or "
                "number of BeginStep calls (streaming), in call to Get");
        }
        for (size_t i = 0; i < variable.m_StepsCount; i++)
        {
            ret = QueueGetSingle(variable, DestData, variable.m_StepsStart + i);
            size_t increment = variable.TotalSize() * variable.m_ElementSize;
            DestData = (void *)((char *)DestData + increment);
        }
        return ret;
    }
}

void BP5Deserializer::GetSingleValueFromMetadata(core::VariableBase &variable,
                                                 BP5VarRec *VarRec,
                                                 void *DestData, size_t Step,
                                                 size_t WriterRank)
{
    char *src = (char *)GetMetadataBase(VarRec, Step, WriterRank);

    if (!src)
        return;

    if (variable.m_SelectionType == adios2::SelectionType::WriteBlock)
        WriterRank = variable.m_BlockID;

    if (variable.m_Type != DataType::String)
    {
        memcpy(DestData, src, variable.m_ElementSize);
    }
    else
    {
        std::string *TmpStr = static_cast<std::string *>(DestData);
        TmpStr->assign(*(const char **)src);
    }
}

bool BP5Deserializer::QueueGetSingle(core::VariableBase &variable,
                                     void *DestData, size_t Step)
{
    BP5VarRec *VarRec = VarByKey[&variable];
    if (VarRec->OrigShapeID == ShapeID::GlobalValue)
    {
        int WriterRank = 0;
        GetSingleValueFromMetadata(variable, VarRec, DestData, Step,
                                   WriterRank);
        return false;
    }
    if (VarRec->OrigShapeID == ShapeID::LocalValue)
    {
        // Shows up as global array with one element per writer rank
        DestData = (char *)DestData + variable.m_Start[0] * VarRec->ElementSize;
        for (size_t WriterRank = variable.m_Start[0];
             WriterRank < variable.m_Count[0] + variable.m_Start[0];
             WriterRank++)
        {
            GetSingleValueFromMetadata(variable, VarRec, DestData, Step,
                                       WriterRank);
            DestData = (char *)DestData +
                       variable.m_ElementSize; // use variable.m_ElementSize
                                               // because it's the size in local
                                               // memory, VarRec->ElementSize is
                                               // the size in metadata
        }
        return false;
    }
    if ((variable.m_SelectionType == adios2::SelectionType::BoundingBox) &&
        (variable.m_ShapeID == ShapeID::GlobalArray))
    {
        BP5ArrayRequest Req;
        Req.VarRec = VarRec;
        Req.RequestType = Global;
        Req.BlockID = variable.m_BlockID;
        Req.Count = variable.m_Count;
        Req.Start = variable.m_Start;
        Req.Step = Step;
        Req.Data = DestData;
        PendingRequests.push_back(Req);
    }
    else if ((variable.m_SelectionType == adios2::SelectionType::WriteBlock) ||
             (variable.m_ShapeID == ShapeID::LocalArray))
    {
        BP5ArrayRequest Req;
        Req.VarRec = VarByKey[&variable];
        Req.RequestType = Local;
        Req.BlockID = variable.m_BlockID;
        Req.Count = variable.m_Count;
        if (variable.m_SelectionType == adios2::SelectionType::BoundingBox)
        {
            Req.Start = variable.m_Start;
        }
        Req.Data = DestData;
        Req.Step = Step;
        PendingRequests.push_back(Req);
    }
    else
    {
    }
    return true;
}

bool BP5Deserializer::NeedWriter(BP5ArrayRequest Req, size_t WriterRank,
                                 size_t &NodeFirst)
{
    MetaArrayRec *writer_meta_base =
        (MetaArrayRec *)GetMetadataBase(Req.VarRec, Req.Step, WriterRank);

    if (!writer_meta_base)
        return false;

    if (Req.RequestType == Local)
    {
        size_t WriterBlockCount =
            writer_meta_base->Dims
                ? writer_meta_base->DBCount / writer_meta_base->Dims
                : 1;
        if (m_RandomAccessMode)
        {
            //  Not ideal, but we don't keep this around for every var in random
            //  access mode, so calc from scratch
            NodeFirst = 0;
            for (size_t TmpRank = 0; TmpRank < WriterRank; TmpRank++)
            {
                ControlInfo *TmpCI =
                    m_ControlArray[Req.Step][TmpRank]; // writer control array

                size_t MetadataFieldOffset =
                    (*TmpCI->MetaFieldOffset)[Req.VarRec->VarNum];
                MetaArrayRec *tmp_meta_base =
                    (MetaArrayRec
                         *)(((char *)(*MetadataBaseArray[Req.Step])[TmpRank]) +
                            MetadataFieldOffset);
                size_t TmpBlockCount =
                    tmp_meta_base->Dims
                        ? tmp_meta_base->DBCount / tmp_meta_base->Dims
                        : 1;
                NodeFirst += TmpBlockCount;
            }
        }
        else
        {
            NodeFirst = Req.VarRec->PerWriterBlockStart[WriterRank];
        }
        size_t NodeLast = WriterBlockCount + NodeFirst - 1;
        bool res = (NodeFirst <= Req.BlockID) && (NodeLast >= Req.BlockID);
        return res;
    }
    // else Global case
    for (size_t i = 0; i < writer_meta_base->BlockCount; i++)
    {
        for (size_t j = 0; j < writer_meta_base->Dims; j++)
        {
            size_t SelOffset = Req.Start[j];
            size_t SelSize = Req.Count[j];
            size_t RankOffset;
            size_t RankSize;

            RankOffset =
                writer_meta_base->Offsets[i * writer_meta_base->Dims + j];
            RankSize = writer_meta_base->Count[i * writer_meta_base->Dims + j];
            if ((SelSize == 0) || (RankSize == 0))
            {
                return false;
            }
            if ((RankOffset < SelOffset &&
                 (RankOffset + RankSize) <= SelOffset) ||
                (RankOffset >= SelOffset + SelSize))
            {
                return false;
            }
        }
    }
    return true;
}

std::vector<BP5Deserializer::ReadRequest>
BP5Deserializer::GenerateReadRequests()
{
    std::vector<BP5Deserializer::ReadRequest> Ret;
    std::vector<FFSReaderPerWriterRec> WriterInfo(m_WriterCohortSize);
    typedef std::pair<size_t, size_t> pair;
    std::map<pair, bool> WriterTSNeeded;

    for (const auto &Req : PendingRequests)
    {
        for (size_t i = 0; i < m_WriterCohortSize; i++)
        {
            if (WriterTSNeeded.count(std::make_pair(Req.Step, i)) == 0)
            {
                WriterTSNeeded[std::make_pair(Req.Step, i)] = true;
            }
        }
    }

    for (std::pair<pair, bool> element : WriterTSNeeded)
    {
        ReadRequest RR;
        RR.Timestep = element.first.first;
        RR.WriterRank = element.first.second;
        RR.StartOffset = 0;
        if (m_RandomAccessMode)
        {
            RR.ReadLength =
                ((struct BP5MetadataInfoStruct *)((
                     *MetadataBaseArray[RR.Timestep])[RR.WriterRank]))
                    ->DataBlockSize;
        }
        else
        {
            RR.ReadLength = ((struct BP5MetadataInfoStruct
                                  *)(*m_MetadataBaseAddrs)[RR.WriterRank])
                                ->DataBlockSize;
        }
        RR.DestinationAddr = (char *)malloc(RR.ReadLength);
        RR.Internal = NULL;
        Ret.push_back(RR);
    }
    return Ret;
}

void BP5Deserializer::FinalizeGets(std::vector<ReadRequest> Requests)
{
    for (const auto &Req : PendingRequests)
    {
        //        ImplementGapWarning(Reqs);
        for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize;
             WriterRank++)
        {
            size_t NodeFirst = 0;
            if (NeedWriter(Req, WriterRank, NodeFirst))
            {
                /* if needed this writer fill destination with acquired data */
                int ElementSize = Req.VarRec->ElementSize;
                size_t *GlobalDimensions = Req.VarRec->GlobalDims;
                MetaArrayRec *writer_meta_base =
                    (MetaArrayRec *)GetMetadataBase(Req.VarRec, Req.Step,
                                                    WriterRank);
                if (!writer_meta_base)
                    continue; // Not writen on this step

                int DimCount = writer_meta_base->Dims;
                for (size_t i = 0; i < writer_meta_base->BlockCount; i++)
                {
                    size_t *RankOffset =
                        &writer_meta_base->Offsets[i * writer_meta_base->Dims];
                    const size_t *RankSize =
                        &writer_meta_base->Count[i * writer_meta_base->Dims];
                    std::vector<size_t> ZeroSel(DimCount);
                    std::vector<size_t> ZeroRankOffset(DimCount);
                    std::vector<size_t> ZeroGlobalDimensions(DimCount);
                    const size_t *SelOffset = NULL;
                    const size_t *SelSize = Req.Count.data();
                    int ReqIndex = 0;
                    while (Requests[ReqIndex].WriterRank !=
                               static_cast<size_t>(WriterRank) ||
                           (Requests[ReqIndex].Timestep != Req.Step))
                        ReqIndex++;
                    if (writer_meta_base->DataLocation == NULL)
                    {
                        // No Data from this writer
                        continue;
                    }
                    char *IncomingData =
                        (char *)Requests[ReqIndex].DestinationAddr +
                        writer_meta_base->DataLocation[i];
                    if (Req.Start.size())
                    {
                        SelOffset = Req.Start.data();
                    }
                    if (Req.RequestType == Local)
                    {
                        int LocalBlockID = Req.BlockID - NodeFirst;
                        IncomingData =
                            (char *)Requests[ReqIndex].DestinationAddr +
                            writer_meta_base->DataLocation[LocalBlockID];

                        RankOffset = ZeroRankOffset.data();
                        GlobalDimensions = ZeroGlobalDimensions.data();
                        if (SelOffset == NULL)
                        {
                            SelOffset = ZeroSel.data();
                        }
                        for (int i = 0; i < DimCount; i++)
                        {
                            GlobalDimensions[i] = RankSize[i];
                        }
                    }
                    if (m_ReaderIsRowMajor)
                    {
                        ExtractSelectionFromPartialRM(
                            ElementSize, DimCount, GlobalDimensions, RankOffset,
                            RankSize, SelOffset, SelSize, IncomingData,
                            (char *)Req.Data);
                    }
                    else
                    {
                        ExtractSelectionFromPartialCM(
                            ElementSize, DimCount, GlobalDimensions, RankOffset,
                            RankSize, SelOffset, SelSize, IncomingData,
                            (char *)Req.Data);
                    }
                }
            }
        }
    }
    for (const auto &Req : Requests)
    {
        free((char *)Req.DestinationAddr);
    }
    PendingRequests.clear();
}

void BP5Deserializer::MapGlobalToLocalIndex(size_t Dims,
                                            const size_t *GlobalIndex,
                                            const size_t *LocalOffsets,
                                            size_t *LocalIndex)
{
    for (size_t i = 0; i < Dims; i++)
    {
        LocalIndex[i] = GlobalIndex[i] - LocalOffsets[i];
    }
}

int BP5Deserializer::FindOffset(size_t Dims, const size_t *Size,
                                const size_t *Index)
{
    int Offset = 0;
    for (size_t i = 0; i < Dims; i++)
    {
        Offset = Index[i] + (Size[i] * Offset);
    }
    return Offset;
}

static int FindOffsetCM(size_t Dims, const size_t *Size, const size_t *Index)
{
    size_t Offset = 0;
    for (int i = static_cast<int>(Dims - 1); i >= 0; i--)
    {
        Offset = Index[i] + (Size[i] * Offset);
    }

    return std::min(static_cast<size_t>(INT_MAX), Offset);
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

/*
 * *******************************
 *
 * ExtractSelectionFromPartial*M both need to be extended to work when
 * the reader and writer have different byte orders.  This involves at
 * least supporting simple big/little-endian byte reversal, but a true
 * archival format should also consider mixed and middle-endian
 * hybrids.  This would require changes to the BP5 header so that the
 * appropriate transformations could be determined.
 *
 * *******************************
 */

// Row major version
void BP5Deserializer::ExtractSelectionFromPartialRM(
    int ElementSize, size_t Dims, const size_t *GlobalDims,
    const size_t *PartialOffsets, const size_t *PartialCounts,
    const size_t *SelectionOffsets, const size_t *SelectionCounts,
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
    OperantElementSize = static_cast<size_t>(ElementSize);
    for (int Dim = static_cast<int>(Dims - 1); Dim >= 0; Dim--)
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
    size_t *FirstIndex = (size_t *)malloc(Dims * sizeof(FirstIndex[0]));
    for (size_t Dim = 0; Dim < Dims; Dim++)
    {
        size_t Left = MAX(PartialOffsets[Dim], SelectionOffsets[Dim]);
        size_t Right = MIN(PartialOffsets[Dim] + PartialCounts[Dim],
                           SelectionOffsets[Dim] + SelectionCounts[Dim]);
        if ((OperantDims != 0) && (Dim < OperantDims - 1))
        {
            BlockCount *= (Right - Left);
        }
        FirstIndex[Dim] = Left;
    }
    size_t *SelectionIndex = (size_t *)malloc(Dims * sizeof(SelectionIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, SelectionOffsets, SelectionIndex);
    DestBlockStartOffset = FindOffset(Dims, SelectionCounts, SelectionIndex);
    free(SelectionIndex);
    DestBlockStartOffset *= ElementSize;

    size_t *PartialIndex = (size_t *)malloc(Dims * sizeof(PartialIndex[0]));
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

// Column-major version
void BP5Deserializer::ExtractSelectionFromPartialCM(
    int ElementSize, size_t Dims, const size_t *GlobalDims,
    const size_t *PartialOffsets, const size_t *PartialCounts,
    const size_t *SelectionOffsets, const size_t *SelectionCounts,
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
    for (size_t Dim = 0; Dim < Dims; Dim++)
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
    size_t *FirstIndex = (size_t *)malloc(Dims * sizeof(FirstIndex[0]));
    for (size_t Dim = 0; Dim < Dims; Dim++)
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
    size_t *SelectionIndex = (size_t *)malloc(Dims * sizeof(SelectionIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, SelectionOffsets, SelectionIndex);
    DestBlockStartOffset = FindOffsetCM(Dims, SelectionCounts, SelectionIndex);
    free(SelectionIndex);
    DestBlockStartOffset *= OperantElementSize;

    size_t *PartialIndex = (size_t *)malloc(Dims * sizeof(PartialIndex[0]));
    MapGlobalToLocalIndex(Dims, FirstIndex, PartialOffsets, PartialIndex);
    SourceBlockStartOffset = FindOffsetCM(Dims, PartialCounts, PartialIndex);

    free(PartialIndex);
    SourceBlockStartOffset *= OperantElementSize;

    InData += SourceBlockStartOffset;
    OutData += DestBlockStartOffset;
    for (int i = 0; i < BlockCount; i++)
    {
        memcpy(OutData, InData, BlockSize * ElementSize);
        InData += SourceBlockStride;
        OutData += DestBlockStride;
    }
    free(FirstIndex);
}

BP5Deserializer::BP5Deserializer(int WriterCount, bool WriterIsRowMajor,
                                 bool ReaderIsRowMajor, bool RandomAccessMode)
: m_WriterIsRowMajor{WriterIsRowMajor}, m_ReaderIsRowMajor{ReaderIsRowMajor},
  m_WriterCohortSize{static_cast<size_t>(WriterCount)}, m_RandomAccessMode{
                                                            RandomAccessMode}
{
    FMContext Tmp = create_local_FMcontext();
    ReaderFFSContext = create_FFSContext_FM(Tmp);
    free_FMcontext(Tmp);
}

BP5Deserializer::~BP5Deserializer()
{
    struct ControlInfo *tmp = ControlBlocks;
    free_FFSContext(ReaderFFSContext);
    ControlBlocks = NULL;
    while (tmp)
    {
        struct ControlInfo *next = tmp->Next;
        delete tmp->MetaFieldOffset;
        delete tmp->CIVarIndex;
        free(tmp);
        tmp = next;
    }
    for (auto &VarRec : VarByName)
    {
        free(VarRec.second->VarName);
        delete VarRec.second;
    }
    if (m_FreeableMBA)
        delete m_FreeableMBA;
    for (auto &step : MetadataBaseArray)
    {
        delete step;
    }
}

void *BP5Deserializer::GetMetadataBase(BP5VarRec *VarRec, size_t Step,
                                       size_t WriterRank)
{
    MetaArrayRec *writer_meta_base = NULL;
    if (m_RandomAccessMode)
    {
        ControlInfo *CI =
            m_ControlArray[Step][WriterRank]; // writer control array
        if (((*CI->MetaFieldOffset).size() <= VarRec->VarNum) ||
            ((*CI->MetaFieldOffset)[VarRec->VarNum] == 0))
        {
            // Var does not appear in this record
            return NULL;
        }
        size_t CI_VarIndex = (*CI->CIVarIndex)[VarRec->VarNum];
        BP5MetadataInfoStruct *BaseData =
            (BP5MetadataInfoStruct *)(*MetadataBaseArray[Step])[WriterRank];
        if (!BP5BitfieldTest(BaseData, CI_VarIndex))
        {
            // Var appears in CI, but wasn't written on this step
            return NULL;
        }
        size_t MetadataFieldOffset = (*CI->MetaFieldOffset)[VarRec->VarNum];
        writer_meta_base =
            (MetaArrayRec *)(((char *)(*MetadataBaseArray[Step])[WriterRank]) +
                             MetadataFieldOffset);
    }
    else
    {
        if (VarRec->PerWriterMetaFieldOffset[WriterRank] == 0)
        {
            // Writer didn't write this var
            return NULL;
        }
        writer_meta_base =
            (MetaArrayRec *)(((char *)(*m_MetadataBaseAddrs)[WriterRank]) +
                             VarRec->PerWriterMetaFieldOffset[WriterRank]);
    }
    return writer_meta_base;
}

Engine::MinVarInfo *BP5Deserializer::MinBlocksInfo(const VariableBase &Var,
                                                   size_t Step)
{
    BP5VarRec *VarRec = LookupVarByKey((void *)&Var);

    Engine::MinVarInfo *MV =
        new Engine::MinVarInfo(VarRec->DimCount, VarRec->GlobalDims);

    MV->Dims = VarRec->DimCount;
    MV->Shape = VarRec->GlobalDims;
    MV->IsReverseDims =
        ((MV->Dims > 1) && (m_WriterIsRowMajor != m_ReaderIsRowMajor));

    size_t Id = 0;
    for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize; WriterRank++)
    {
        MetaArrayRec *writer_meta_base =
            (MetaArrayRec *)GetMetadataBase(VarRec, Step, WriterRank);
        if (writer_meta_base)
        {
            size_t WriterBlockCount =
                writer_meta_base->Dims
                    ? writer_meta_base->DBCount / writer_meta_base->Dims
                    : 1;
            Id += WriterBlockCount;
        }
    }
    MV->BlocksInfo.reserve(Id);

    Id = 0;
    for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize; WriterRank++)
    {
        MetaArrayRec *writer_meta_base =
            (MetaArrayRec *)GetMetadataBase(VarRec, Step, WriterRank);

        if (!writer_meta_base)
            continue;
        size_t WriterBlockCount =
            MV->Dims ? writer_meta_base->DBCount / MV->Dims : 1;
        for (size_t i = 0; i < WriterBlockCount; i++)
        {
            size_t *Offsets = NULL;
            size_t *Count = NULL;
            if (writer_meta_base->Offsets)
                Offsets = writer_meta_base->Offsets + (i * MV->Dims);
            if (writer_meta_base->Count)
                Count = writer_meta_base->Count + (i * MV->Dims);
            Engine::MinBlockInfo Blk;
            Blk.WriterID = WriterRank;
            Blk.BlockID = Id++;
            Blk.Start = Offsets;
            Blk.Count = Count;
            // Blk.MinUnion
            // Blk.MaxUnion
            // Blk.BufferP
            MV->BlocksInfo.push_back(Blk);
        }
    }
    return MV;
}

void InitMinMax(Engine::MinMaxStruct &MinMax, DataType Type)
{
    switch (Type)
    {
    case DataType::None:
        break;
    case DataType::Char:
        MinMax.MinUnion.field_char = SCHAR_MAX;
        MinMax.MaxUnion.field_char = SCHAR_MIN;
        break;
    case DataType::Int8:
        MinMax.MinUnion.field_int8 = INT8_MAX;
        MinMax.MaxUnion.field_int8 = INT8_MIN;
        break;
    case DataType::Int16:
        MinMax.MinUnion.field_int16 = INT16_MAX;
        MinMax.MaxUnion.field_int16 = INT16_MIN;
        break;
    case DataType::Int32:
        MinMax.MinUnion.field_int32 = INT32_MAX;
        MinMax.MaxUnion.field_int32 = INT32_MIN;
        break;
    case DataType::Int64:
        MinMax.MinUnion.field_int64 = INT64_MAX;
        MinMax.MaxUnion.field_int64 = INT64_MIN;
        break;
    case DataType::UInt8:
        MinMax.MinUnion.field_uint8 = UINT8_MAX;
        MinMax.MaxUnion.field_uint8 = 0;
        break;
    case DataType::UInt16:
        MinMax.MinUnion.field_uint16 = UINT16_MAX;
        MinMax.MaxUnion.field_uint16 = 0;
        break;
    case DataType::UInt32:
        MinMax.MinUnion.field_uint32 = UINT32_MAX;
        MinMax.MaxUnion.field_uint32 = 0;
        break;
    case DataType::UInt64:
        MinMax.MinUnion.field_uint64 = UINT64_MAX;
        MinMax.MaxUnion.field_uint64 = 0;
        break;
    case DataType::Float:
        MinMax.MinUnion.field_float = FLT_MAX;
        MinMax.MaxUnion.field_float = -FLT_MAX;
        break;
    case DataType::Double:
        MinMax.MinUnion.field_double = DBL_MAX;
        MinMax.MaxUnion.field_double = -DBL_MAX;
    case DataType::LongDouble:
        MinMax.MinUnion.field_ldouble = LDBL_MAX;
        MinMax.MaxUnion.field_ldouble = -LDBL_MAX;
        break;
    case DataType::FloatComplex:
    case DataType::DoubleComplex:
    case DataType::String:
    case DataType::Compound:
        break;
    }
}

void ApplyElementMinMax(Engine::MinMaxStruct &MinMax, DataType Type,
                        void *Element)
{
    switch (Type)
    {
    case DataType::None:
        break;
    case DataType::Char:
        if (*(char *)Element < MinMax.MinUnion.field_char)
            MinMax.MinUnion.field_char = *(char *)Element;
        if (*(char *)Element > MinMax.MaxUnion.field_char)
            MinMax.MaxUnion.field_char = *(char *)Element;
        break;
    case DataType::Int8:
        if (*(int8_t *)Element < MinMax.MinUnion.field_int8)
            MinMax.MinUnion.field_int8 = *(int8_t *)Element;
        if (*(int8_t *)Element > MinMax.MaxUnion.field_int8)
            MinMax.MaxUnion.field_int8 = *(int8_t *)Element;
        break;
    case DataType::Int16:
        if (*(int16_t *)Element < MinMax.MinUnion.field_int16)
            MinMax.MinUnion.field_int16 = *(int16_t *)Element;
        if (*(int16_t *)Element > MinMax.MaxUnion.field_int16)
            MinMax.MaxUnion.field_int16 = *(int16_t *)Element;
        break;
    case DataType::Int32:
        if (*(int32_t *)Element < MinMax.MinUnion.field_int32)
            MinMax.MinUnion.field_int32 = *(int32_t *)Element;
        if (*(int32_t *)Element > MinMax.MaxUnion.field_int32)
            MinMax.MaxUnion.field_int32 = *(int32_t *)Element;
        break;
    case DataType::Int64:
        if (*(int64_t *)Element < MinMax.MinUnion.field_int64)
            MinMax.MinUnion.field_int64 = *(int64_t *)Element;
        if (*(int64_t *)Element > MinMax.MaxUnion.field_int64)
            MinMax.MaxUnion.field_int64 = *(int64_t *)Element;
        break;
    case DataType::UInt8:
        if (*(uint8_t *)Element < MinMax.MinUnion.field_uint8)
            MinMax.MinUnion.field_uint8 = *(uint8_t *)Element;
        if (*(uint8_t *)Element > MinMax.MaxUnion.field_uint8)
            MinMax.MaxUnion.field_uint8 = *(uint8_t *)Element;
        break;
    case DataType::UInt16:
        if (*(uint16_t *)Element < MinMax.MinUnion.field_uint16)
            MinMax.MinUnion.field_uint16 = *(uint16_t *)Element;
        if (*(uint16_t *)Element > MinMax.MaxUnion.field_uint16)
            MinMax.MaxUnion.field_uint16 = *(uint16_t *)Element;
        break;
    case DataType::UInt32:
        if (*(uint32_t *)Element < MinMax.MinUnion.field_uint32)
            MinMax.MinUnion.field_uint32 = *(uint32_t *)Element;
        if (*(uint32_t *)Element > MinMax.MaxUnion.field_uint32)
            MinMax.MaxUnion.field_uint32 = *(uint32_t *)Element;
        break;
    case DataType::UInt64:
        if (*(uint64_t *)Element < MinMax.MinUnion.field_uint64)
            MinMax.MinUnion.field_uint64 = *(uint64_t *)Element;
        if (*(uint64_t *)Element > MinMax.MaxUnion.field_uint64)
            MinMax.MaxUnion.field_uint64 = *(uint64_t *)Element;
        break;
    case DataType::Float:
        if (*(float *)Element < MinMax.MinUnion.field_float)
            MinMax.MinUnion.field_float = *(float *)Element;
        if (*(float *)Element > MinMax.MaxUnion.field_float)
            MinMax.MaxUnion.field_float = *(float *)Element;
        break;
    case DataType::Double:
        if (*(double *)Element < MinMax.MinUnion.field_double)
            MinMax.MinUnion.field_double = *(double *)Element;
        if (*(double *)Element > MinMax.MaxUnion.field_double)
            MinMax.MaxUnion.field_double = *(double *)Element;
        break;
    case DataType::LongDouble:
        if (*(long double *)Element < MinMax.MinUnion.field_ldouble)
            MinMax.MinUnion.field_ldouble = *(long double *)Element;
        if (*(long double *)Element > MinMax.MaxUnion.field_ldouble)
            MinMax.MaxUnion.field_ldouble = *(long double *)Element;
        break;
    case DataType::FloatComplex:
    case DataType::DoubleComplex:
    case DataType::String:
    case DataType::Compound:
        break;
    }
}

size_t BP5Deserializer::RelativeToAbsoluteStep(const BP5VarRec *VarRec,
                                               size_t RelStep)
{
    //  Consider an optimization here.  Track the number of timesteps
    //  available to the engine and the number of steps upon which a
    //  variable appears.  If the first step it appears on plus the
    //  number of steps it appears adds up to the number of steps
    //  available to the engine, then there are no gaps and we can
    //  easily calculate the RelativeToAbsoluteStep transformation
    //  without checking.  That's probably the most common case.
    //  But for now, the simple stupid solution
    size_t AbsStep = VarRec->FirstTSSeen;
    while (RelStep != 0)
    {
        size_t WriterRank = 0;
        while (WriterRank < m_WriterCohortSize)
        {
            BP5MetadataInfoStruct *BaseData;
            BaseData = (BP5MetadataInfoStruct
                            *)(*MetadataBaseArray[AbsStep])[WriterRank];
            if (BP5BitfieldTest((BP5MetadataInfoStruct *)BaseData,
                                VarRec->VarNum))
            {
                // variable appeared on this step
                RelStep--;
                break; // exit while (WriterRank < m_WriterCohortSize)
            }
            WriterRank++;
        }
        AbsStep++;
    }
    return AbsStep;
}

bool BP5Deserializer::VariableMinMax(const VariableBase &Var, const size_t Step,
                                     Engine::MinMaxStruct &MinMax)
{
    BP5VarRec *VarRec = LookupVarByKey((void *)&Var);
    InitMinMax(MinMax, VarRec->Type);

    size_t StartStep = Step, StopStep = Step + 1;
    if (Step == DefaultSizeT)
    {
        StartStep = 0;
        StopStep = m_ControlArray.size();
        if (!m_RandomAccessMode)
            StopStep = 1;
    }
    for (size_t RelStep = StartStep; RelStep < StopStep; RelStep++)
    {
        if ((VarRec->OrigShapeID == ShapeID::LocalValue) ||
            (VarRec->OrigShapeID == ShapeID::GlobalValue))
        {
            for (size_t WriterRank = 0; WriterRank < m_WriterCohortSize;
                 WriterRank++)
            {
                void *writer_meta_base;
                BP5MetadataInfoStruct *BaseData;
                if (m_RandomAccessMode)
                {
                    size_t AbsStep = RelativeToAbsoluteStep(VarRec, RelStep);
                    if (AbsStep >= m_ControlArray.size())
                        return true; // done
                    ControlInfo *CI =
                        m_ControlArray[AbsStep]
                                      [WriterRank]; // writer 0 control array
                    size_t MetadataFieldOffset =
                        (*CI->MetaFieldOffset)[VarRec->VarNum];
                    BaseData = (BP5MetadataInfoStruct
                                    *)(*MetadataBaseArray[AbsStep])[WriterRank];
                    writer_meta_base =
                        (MetaArrayRec *)(((char *)(*MetadataBaseArray[AbsStep])
                                              [WriterRank]) +
                                         MetadataFieldOffset);
                }
                else
                {
                    BaseData = (BP5MetadataInfoStruct
                                    *)(*m_MetadataBaseAddrs)[WriterRank];
                    writer_meta_base =
                        (MetaArrayRec
                             *)(((char *)(*m_MetadataBaseAddrs)[WriterRank]) +
                                VarRec->PerWriterMetaFieldOffset[WriterRank]);
                }
                if (BP5BitfieldTest(BaseData, VarRec->VarNum))
                {
                    ApplyElementMinMax(MinMax, VarRec->Type, writer_meta_base);
                }
            }
        }
    }
    return true;
}

}
}
