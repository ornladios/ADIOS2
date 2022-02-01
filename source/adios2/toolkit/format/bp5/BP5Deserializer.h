/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Deserializer.h
 *
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP5_BP5DESERIALIZER_H_

#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"

#include "BP5Base.h"
#include "atl.h"
#include "ffs.h"
#include "fm.h"

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace adios2
{
namespace format
{

using namespace core;

class BP5Deserializer : virtual public BP5Base
{

public:
    BP5Deserializer(bool WriterIsRowMajor, bool ReaderIsRowMajor,
                    bool RandomAccessMode = false);

    ~BP5Deserializer();

    struct ReadRequest
    {
        size_t Timestep;
        size_t WriterRank;
        size_t StartOffset;
        size_t ReadLength;
        char *DestinationAddr;
        void *Internal;
    };
    void InstallMetaMetaData(MetaMetaInfoBlock &MMList);
    void InstallMetaData(void *MetadataBlock, size_t BlockLen,
                         size_t WriterRank, size_t Step = SIZE_MAX);
    void InstallAttributeData(void *AttributeBlock, size_t BlockLen,
                              size_t Step = SIZE_MAX);
    void SetupForStep(size_t Step, size_t WriterCount);
    // return from QueueGet is true if a sync is needed to fill the data
    bool QueueGet(core::VariableBase &variable, void *DestData);
    bool QueueGetSingle(core::VariableBase &variable, void *DestData,
                        size_t Step);

    std::vector<ReadRequest> GenerateReadRequests();
    void FinalizeGets(std::vector<ReadRequest>);

    MinVarInfo *AllRelativeStepsMinBlocksInfo(const VariableBase &var);
    MinVarInfo *AllStepsMinBlocksInfo(const VariableBase &var);
    MinVarInfo *MinBlocksInfo(const VariableBase &Var, const size_t Step);
    bool VariableMinMax(const VariableBase &var, const size_t Step,
                        MinMaxStruct &MinMax);
    void GetAbsoluteSteps(const VariableBase &variable,
                          std::vector<size_t> &keys) const;

    const bool m_WriterIsRowMajor;
    const bool m_ReaderIsRowMajor;
    core::Engine *m_Engine = NULL;

private:
    size_t m_VarCount = 0;
    struct BP5VarRec
    {
        size_t VarNum;
        void *Variable = NULL;
        char *VarName = NULL;
        size_t DimCount = 0;
        ShapeID OrigShapeID;
        char *Operator = NULL;
        DataType Type;
        int ElementSize = 0;
        size_t MinMaxOffset = SIZE_MAX;
        size_t *GlobalDims = NULL;
        size_t LastTSAdded = SIZE_MAX;
        size_t FirstTSSeen = SIZE_MAX;
        size_t LastShapeAdded = SIZE_MAX;
        std::vector<size_t> PerWriterMetaFieldOffset;
        std::vector<size_t> PerWriterBlockStart;
    };

    struct ControlStruct
    {
        int FieldOffset;
        BP5VarRec *VarRec;
        ShapeID OrigShapeID;
        DataType Type;
        int ElementSize;
    };

    struct ControlInfo
    {
        FMFormat Format;
        int ControlCount;
        struct ControlInfo *Next;
        std::vector<size_t> *MetaFieldOffset;
        std::vector<size_t> *CIVarIndex;
        struct ControlStruct Controls[1];
    };

    enum WriterDataStatusEnum
    {
        Empty = 0,
        Needed = 1,
        Requested = 2,
        Full = 3
    };

    struct FFSReaderPerWriterRec
    {
        enum WriterDataStatusEnum Status = Empty;
    };

    FFSContext ReaderFFSContext;

    const bool m_RandomAccessMode;

    std::vector<size_t> m_WriterCohortSize; // per step, in random mode
    size_t m_CurrentWriterCohortSize;       // valid in streaming mode
    // return the number of writers
    // m_CurrentWriterCohortSize in streaming mode
    // m_WriterCohortSize[Step] in random access mode
    size_t WriterCohortSize(size_t Step) const;

    size_t m_LastAttrStep = MaxSizeT; // invalid timestep for start

    std::unordered_map<std::string, BP5VarRec *> VarByName;
    std::unordered_map<void *, BP5VarRec *> VarByKey;

    std::vector<void *> *m_MetadataBaseAddrs =
        nullptr; // may be a pointer into MetadataBaseArray or m_FreeableMBA
    std::vector<void *> *m_FreeableMBA = nullptr;

    // for random access mode, for each timestep, for each writerrank, what
    // metameta info applies to the metadata
    std::vector<std::vector<ControlInfo *>> m_ControlArray;
    // for random access mode, for each timestep, for each writerrank, base
    // address of the metadata
    std::vector<std::vector<void *> *> MetadataBaseArray;

    ControlInfo *ControlBlocks = nullptr;
    ControlInfo *GetPriorControl(FMFormat Format);
    ControlInfo *BuildControl(FMFormat Format);
    bool NameIndicatesArray(const char *Name);
    bool NameIndicatesAttrArray(const char *Name);
    DataType TranslateFFSType2ADIOS(const char *Type, int size);
    BP5VarRec *LookupVarByKey(void *Key) const;
    BP5VarRec *LookupVarByName(const char *Name);
    BP5VarRec *CreateVarRec(const char *ArrayName);
    void ReverseDimensions(size_t *Dimensions, int count, int times);
    void BreakdownVarName(const char *Name, char **base_name_p,
                          DataType *type_p, int *element_size_p);
    void BreakdownArrayName(const char *Name, char **base_name_p,
                            DataType *type_p, int *element_size_p,
                            char **Operator, bool *MinMax);
    void *VarSetup(core::Engine *engine, const char *variableName,
                   const DataType type, void *data);
    void *ArrayVarSetup(core::Engine *engine, const char *variableName,
                        const DataType type, int DimCount, size_t *Shape,
                        size_t *Start, size_t *Count);
    void MapGlobalToLocalIndex(size_t Dims, const size_t *GlobalIndex,
                               const size_t *LocalOffsets, size_t *LocalIndex);
    size_t RelativeToAbsoluteStep(const BP5VarRec *VarRec, size_t RelStep);
    int FindOffset(size_t Dims, const size_t *Size, const size_t *Index);
    bool GetSingleValueFromMetadata(core::VariableBase &variable,
                                    BP5VarRec *VarRec, void *DestData,
                                    size_t Step, size_t WriterRank);
    void MemCopyData(char *OutData, const char *InData, size_t Size,
                     MemorySpace MemSpace);
    void ExtractSelectionFromPartialRM(
        int ElementSize, size_t Dims, const size_t *GlobalDims,
        const size_t *PartialOffsets, const size_t *PartialCounts,
        const size_t *SelectionOffsets, const size_t *SelectionCounts,
        const char *InData, char *OutData,
        MemorySpace MemSpace = MemorySpace::Host);
    void ExtractSelectionFromPartialCM(
        int ElementSize, size_t Dims, const size_t *GlobalDims,
        const size_t *PartialOffsets, const size_t *PartialCounts,
        const size_t *SelectionOffsets, const size_t *SelectionCounts,
        const char *InData, char *OutData,
        MemorySpace MemSpace = MemorySpace::Host);

    enum RequestTypeEnum
    {
        Global = 0,
        Local = 1
    };

    struct BP5ArrayRequest
    {
        BP5VarRec *VarRec = NULL;
        enum RequestTypeEnum RequestType;
        size_t Step;
        size_t BlockID;
        Dims Start;
        Dims Count;
        MemorySpace MemSpace;
        void *Data;
    };
    std::vector<BP5ArrayRequest> PendingRequests;
    bool NeedWriter(BP5ArrayRequest Req, size_t i, size_t &NodeFirst);
    void *GetMetadataBase(BP5VarRec *VarRec, size_t Step,
                          size_t WriterRank) const;
    size_t CurTimestep = 0;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP5_BP5Serializer_H_ */
