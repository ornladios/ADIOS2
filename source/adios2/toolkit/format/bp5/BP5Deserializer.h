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

class BP5Deserializer : virtual public BP5Base
{

public:
    BP5Deserializer(int WriterCount, bool WriterIsRowMajor,
                    bool ReaderIsRowMajor);

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
                         size_t WriterRank);
    void InstallAttributeData(void *AttributeBlock, size_t BlockLen);
    void SetupForTimestep(size_t t);
    // return from QueueGet is true if a sync is needed to fill the data
    bool QueueGet(core::VariableBase &variable, void *DestData);

    std::vector<ReadRequest> GenerateReadRequests();
    void FinalizeGets(std::vector<ReadRequest>);

    bool m_WriterIsRowMajor = 1;
    bool m_ReaderIsRowMajor = 1;
    core::Engine *m_Engine = NULL;

    template <class T>
    std::vector<typename core::Variable<T>::BPInfo>
    BlocksInfo(const core::Variable<T> &variable, const size_t step) const;

private:
    struct BP5VarRec
    {
        void *Variable = NULL;
        char *VarName = NULL;
        size_t DimCount = 0;
        DataType Type;
        int ElementSize = 0;
        size_t *GlobalDims = NULL;
        std::vector<size_t> PerWriterMetaFieldOffset;
        std::vector<size_t> PerWriterBlockStart;
        std::vector<size_t> PerWriterBlockCount;
        std::vector<size_t *> PerWriterStart;
        std::vector<size_t *> PerWriterCounts;
        std::vector<void *> PerWriterIncomingData;
        std::vector<size_t> PerWriterIncomingSize; // important for compression
        std::vector<size_t *> PerWriterDataLocation;
        BP5VarRec(int WriterSize)
        {
            PerWriterMetaFieldOffset.resize(WriterSize);
            PerWriterBlockStart.resize(WriterSize);
            PerWriterBlockCount.resize(WriterSize);
            PerWriterStart.resize(WriterSize);
            PerWriterCounts.resize(WriterSize);
            PerWriterIncomingData.resize(WriterSize);
            PerWriterIncomingSize.resize(WriterSize);
            PerWriterDataLocation.resize(WriterSize);
        }
    };

    struct ControlStruct
    {
        int FieldIndex;
        int FieldOffset;
        BP5VarRec *VarRec;
        int IsArray;
        DataType Type;
        int ElementSize;
    };

    struct ControlInfo
    {
        FMFormat Format;
        int ControlCount;
        struct ControlInfo *Next;
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
        char *RawBuffer = NULL;
    };

    FFSContext ReaderFFSContext;
    int m_WriterCohortSize;
    std::unordered_map<std::string, BP5VarRec *> VarByName;
    std::unordered_map<void *, BP5VarRec *> VarByKey;
    FMContext LocalFMContext;
    //    Ffsarrayrequest PendingVarRequests;

    std::vector<void *> MetadataBaseAddrs;
    std::vector<FMFieldList> MetadataFieldLists;
    std::vector<void *> DataBaseAddrs;
    std::vector<FFSReaderPerWriterRec> WriterInfo;
    //  struct ControlInfo *ControlBlocks;

    ControlInfo *ControlBlocks = nullptr;
    ControlInfo *GetPriorControl(FMFormat Format);
    ControlInfo *BuildControl(FMFormat Format);
    bool NameIndicatesArray(const char *Name);
    DataType TranslateFFSType2ADIOS(const char *Type, int size);
    BP5VarRec *LookupVarByKey(void *Key);
    BP5VarRec *LookupVarByName(const char *Name);
    BP5VarRec *CreateVarRec(const char *ArrayName);
    void ReverseDimensions(size_t *Dimensions, int count);
    void BreakdownVarName(const char *Name, char **base_name_p,
                          DataType *type_p, int *element_size_p);
    void BreakdownArrayName(const char *Name, char **base_name_p,
                            DataType *type_p, int *element_size_p);
    void *VarSetup(core::Engine *engine, const char *variableName,
                   const DataType type, void *data);
    void *ArrayVarSetup(core::Engine *engine, const char *variableName,
                        const DataType type, int DimCount, size_t *Shape,
                        size_t *Start, size_t *Count);
    void MapGlobalToLocalIndex(size_t Dims, const size_t *GlobalIndex,
                               const size_t *LocalOffsets, size_t *LocalIndex);
    int FindOffset(size_t Dims, const size_t *Size, const size_t *Index);
    void ExtractSelectionFromPartialRM(int ElementSize, size_t Dims,
                                       const size_t *GlobalDims,
                                       const size_t *PartialOffsets,
                                       const size_t *PartialCounts,
                                       const size_t *SelectionOffsets,
                                       const size_t *SelectionCounts,
                                       const char *InData, char *OutData);
    void ExtractSelectionFromPartialCM(int ElementSize, size_t Dims,
                                       const size_t *GlobalDims,
                                       const size_t *PartialOffsets,
                                       const size_t *PartialCounts,
                                       const size_t *SelectionOffsets,
                                       const size_t *SelectionCounts,
                                       const char *InData, char *OutData);

    enum RequestTypeEnum
    {
        Global = 0,
        Local = 1
    };

    struct BP5ArrayRequest
    {
        BP5VarRec *VarRec = NULL;
        enum RequestTypeEnum RequestType;
        size_t BlockID;
        Dims Start;
        Dims Count;
        void *Data;
    };
    std::vector<BP5ArrayRequest> PendingRequests;
    bool NeedWriter(BP5ArrayRequest Req, int i);
    size_t CurTimestep = 0;
    std::vector<struct ControlInfo *> ActiveControl;
};

#define declare_template_instantiation(T)                                      \
    extern template std::vector<typename core::Variable<T>::BPInfo>            \
    BP5Deserializer::BlocksInfo(const core::Variable<T> &, const size_t)       \
        const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP5_BP5Serializer_H_ */
