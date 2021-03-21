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

BP5Deserializer::BP5VarRec *BP5Deserializer::LookupVarByKey(void *Key)
{
    auto ret = VarByKey[Key];
    std::cout << "Lookup var by key for key " << Key << " returning " << ret
              << std::endl;

    return ret;
}

BP5Deserializer::BP5VarRec *BP5Deserializer::LookupVarByName(const char *Name)
{
    auto ret = VarByName[Name];
    std::cout << "Lookup var by name for name " << Name << " returning " << ret
              << std::endl;

    return ret;
}

BP5Deserializer::BP5VarRec *BP5Deserializer::CreateVarRec(const char *ArrayName)
{
    BP5VarRec *Ret = new BP5VarRec(WriterCohortSize);
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
            BP5VarRec *VarRec = nullptr;
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
            BP5VarRec *VarRec = NULL;
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

void BP5Deserializer::SetupForTimestep(size_t Timestep)
{
    CurTimestep = Timestep;
    PendingRequests.clear();
    for (auto RecPair : VarByKey)
    {
        RecPair.second->Variable = NULL;
    }
}
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
        BP5VarRec *VarRec = ControlArray[i].VarRec;
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
                VarByKey[VarRec->Variable] = VarRec;
            }
            VarRec->DimCount = meta_base->Dims;
            VarRec->PerWriterBlockCount[WriterRank] =
                meta_base->Dims ? meta_base->DBCount / meta_base->Dims : 1;
            VarRec->PerWriterStart[WriterRank] = meta_base->Offsets;
            VarRec->PerWriterCounts[WriterRank] = meta_base->Count;
            VarRec->PerWriterDataLocation[WriterRank] = meta_base->DataLocation;
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
                VarByKey[VarRec->Variable] = VarRec;
            }
            VarRec->PerWriterMetaFieldOffset[WriterRank] = FieldOffset;
        }
    }
}

bool BP5Deserializer::QueueGet(core::VariableBase &variable, void *DestData)
{
    if (variable.m_SingleValue)
    {
        int WriterRank = 0;
        if (variable.m_SelectionType == adios2::SelectionType::WriteBlock)
            WriterRank = variable.m_BlockID;

        BP5VarRec *VarRec = VarByKey[&variable];
        char *src = ((char *)MetadataBaseAddrs[WriterRank]) +
                    VarRec->PerWriterMetaFieldOffset[WriterRank];
        memcpy(DestData, src, variable.m_ElementSize);
        return false;
    }
    if (variable.m_SelectionType == adios2::SelectionType::BoundingBox)
    {
        BP5ArrayRequest Req;
        Req.VarRec = VarByKey[&variable];
        Req.RequestType = Global;
        Req.BlockID = variable.m_BlockID;
        Req.Count = variable.m_Count;
        Req.Start = variable.m_Start;
        Req.Data = DestData;
        PendingRequests.push_back(Req);
    }
    else if (variable.m_SelectionType == adios2::SelectionType::WriteBlock)
    {
        BP5ArrayRequest Req;
        Req.VarRec = VarByKey[&variable];
        Req.RequestType = Local;
        Req.BlockID = variable.m_BlockID;
        Req.Count = variable.m_Count;
        Req.Data = DestData;
        PendingRequests.push_back(Req);
    }
    else
    {
    }
    return false;
}

bool BP5Deserializer::NeedWriter(BP5ArrayRequest Req, int i)
{
    if (Req.RequestType == Local)
    {
        size_t NodeFirst = Req.VarRec->PerWriterBlockStart[i];
        size_t NodeLast = Req.VarRec->PerWriterBlockCount[i] + NodeFirst - 1;
        return (NodeFirst <= Req.BlockID) && (NodeLast >= Req.BlockID);
    }
    // else Global case
    for (int j = 0; j < Req.VarRec->DimCount; j++)
    {
        size_t SelOffset = Req.Start[j];
        size_t SelSize = Req.Count[j];
        size_t RankOffset;
        size_t RankSize;
        if (Req.VarRec->PerWriterStart[i] == NULL)
        /* this writer didn't write */
        {
            return false;
        }
        RankOffset = Req.VarRec->PerWriterStart[i][j];
        RankSize = Req.VarRec->PerWriterCounts[i][j];
        if ((SelSize == 0) || (RankSize == 0))
        {
            return false;
        }
        if ((RankOffset < SelOffset && (RankOffset + RankSize) <= SelOffset) ||
            (RankOffset >= SelOffset + SelSize))
        {
            return false;
        }
    }
    return true;
}

std::vector<BP5Deserializer::ReadRequest>
BP5Deserializer::GenerateReadRequests()
{
    std::vector<BP5Deserializer::ReadRequest> Ret;
    WriterInfo.reserve(WriterCohortSize);
    for (auto &W : WriterInfo)
    {
        W.Status = Empty;
        W.RawBuffer = NULL;
    }

    for (const auto &Req : PendingRequests)
    {
        for (int i = 0; i < WriterCohortSize; i++)
        {
            if ((WriterInfo[i].Status != Needed) && (NeedWriter(Req, i)))
            {
                WriterInfo[i].Status = Needed;
            }
        }
    }

    for (int i = 0; i < WriterCohortSize; i++)
    {
        if (WriterInfo[i].Status == Needed)
        {
            ReadRequest RR;
            RR.Timestep = CurTimestep;
            RR.WriterRank = i;
            RR.StartOffset = 0;
            printf("Setting up read for writer %d, length %zu\n", i,
                   ((struct FFSMetadataInfoStruct *)MetadataBaseAddrs[i])
                       ->DataBlockSize);
            RR.ReadLength =
                ((struct FFSMetadataInfoStruct *)MetadataBaseAddrs[i])
                    ->DataBlockSize;
            RR.DestinationAddr = (char *)malloc(RR.ReadLength);
            RR.Internal = NULL;
            Ret.push_back(RR);
        }
    }
    return Ret;
}

void BP5Deserializer::FinalizeGets(std::vector<ReadRequest> Requests)
{
    for (const auto &Req : PendingRequests)
    {
        //        ImplementGapWarning(Reqs);
        for (int i = 0; i < WriterCohortSize; i++)
        {
            printf("Looking at request for var %s , NeedWriter is %d\n",
                   Req.VarRec->VarName, NeedWriter(Req, i));
            if (NeedWriter(Req, i))
            {
                /* if needed this writer fill destination with acquired data */
                int ElementSize = Req.VarRec->ElementSize;
                int DimCount = Req.VarRec->DimCount;
                size_t *GlobalDimensions = Req.VarRec->GlobalDims;
                size_t *RankOffset = Req.VarRec->PerWriterStart[i];
                const size_t *RankSize = Req.VarRec->PerWriterCounts[i];
                std::vector<size_t> ZeroSel(DimCount);
                const size_t *SelOffset = NULL;
                const size_t *SelSize = Req.Count.data();
                DataType Type = Req.VarRec->Type;
                int ReqIndex = 0;
                while (Requests[ReqIndex].WriterRank != i)
                    ReqIndex++;
                char *IncomingData =
                    (char *)Requests[ReqIndex].DestinationAddr +
                    Req.VarRec->PerWriterDataLocation[i][0];
                int FreeIncoming = 0;

                if (Req.Start.size())
                {
                    SelOffset = Req.Start.data();
                }
                if (Req.RequestType == Local)
                {
                    int LocalBlockID =
                        Req.BlockID - Req.VarRec->PerWriterBlockStart[i];
                    size_t DataOffset = 0;
                    for (int i = 0; i < LocalBlockID; i++)
                    {
                        int BlockElemCount = 1;
                        for (int j = 0; j < DimCount; j++)
                        {
                            BlockElemCount *= RankSize[j];
                        }
                        DataOffset += BlockElemCount * ElementSize;
                        RankSize += DimCount;
                    }
                    std::vector<size_t> ZeroRankOffset(DimCount);
                    std::vector<size_t> ZeroGlobalDimensions(DimCount);
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
                    IncomingData = IncomingData + DataOffset;
                }
                printf("Incoming data for this var %s is at %p (%p + %zu)\n",
                       Req.VarRec->VarName, IncomingData,
                       (char *)Requests[i].DestinationAddr,
                       Req.VarRec->PerWriterDataLocation[i][0]);
                if (ReaderIsRowMajor)
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

void BP5Deserializer::MapGlobalToLocalIndex(size_t Dims,
                                            const size_t *GlobalIndex,
                                            const size_t *LocalOffsets,
                                            size_t *LocalIndex)
{
    for (int i = 0; i < Dims; i++)
    {
        LocalIndex[i] = GlobalIndex[i] - LocalOffsets[i];
    }
}

int BP5Deserializer::FindOffset(size_t Dims, const size_t *Size,
                                const size_t *Index)
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
    size_t *FirstIndex = (size_t *)malloc(Dims * sizeof(FirstIndex[0]));
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
    size_t *FirstIndex = (size_t *)malloc(Dims * sizeof(FirstIndex[0]));
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
    printf("Copying %d blocks size %d * %d\n", BlockCount, BlockSize,
           ElementSize);
    for (int i = 0; i < BlockCount; i++)
    {
        for (int j = 0; j < BlockSize * ElementSize; j++)
        {
            printf("%02x ", (unsigned char)InData[j]);
        }
        printf("\n");
        memcpy(OutData, InData, BlockSize * ElementSize);
        InData += SourceBlockStride;
        OutData += DestBlockStride;
    }
    free(FirstIndex);
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
