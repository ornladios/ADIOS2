/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Serializer.h
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP5_BP5SERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP5_BP5SERIALIZER_H_

#include "BP5Base.h"
#include "adios2/core/Attribute.h"
#include "adios2/core/CoreTypes.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/format/buffer/BufferV.h"
#include "adios2/toolkit/format/buffer/heap/BufferSTL.h"
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

class BP5Serializer : virtual public BP5Base
{

public:
    BP5Serializer();
    ~BP5Serializer();

    struct TimestepInfo
    {
        std::vector<MetaMetaInfoBlock> NewMetaMetaBlocks;
        Buffer *MetaEncodeBuffer;
        Buffer *AttributeEncodeBuffer;
        BufferV *DataBuffer;

        ~TimestepInfo()
        {
            delete MetaEncodeBuffer;
            if (AttributeEncodeBuffer)
                delete AttributeEncodeBuffer;
            delete DataBuffer;
        }
    };

    typedef struct _MetadataInfo
    {
        std::vector<MetaMetaInfoBlock> NewMetaMetaBlocks;
        std::vector<size_t> MetaEncodeBufferSizes;
        std::vector<char *> MetaEncodeBuffers;

        std::vector<size_t> AttributeEncodeBufferSizes;
        std::vector<char *> AttributeEncodeBuffers;
        Buffer BackingBuffer;
    } AggregatedMetadataInfo;

    void Marshal(void *Variable, const char *Name, const DataType Type,
                 size_t ElemSize, size_t DimCount, const size_t *Shape,
                 const size_t *Count, const size_t *Offsets, const void *Data,
                 bool Sync, BufferV::BufferPos *span);
    void MarshalAttribute(const char *Name, const DataType Type,
                          size_t ElemSize, size_t ElemCount, const void *Data);

    /*
     *  InitStep must be called with an appropriate BufferV subtype before a
     * step can begin
     */
    void InitStep(BufferV *DataBuffer);

    /*
     * ReinitStepData can be called to "flush" out already written
     * data it returns a BufferV representing already-written data and
     * provides the serializer with a new, empty BufferV This call
     * does *not* reset the data offsets generated with Marshal, so
     * those offsets are relative to the entire sequence of data
     * produced by a writer rank.
     */
    BufferV *ReinitStepData(BufferV *DataBuffer);

    TimestepInfo CloseTimestep(int timestep);
    void PerformPuts();

    core::Engine *m_Engine = NULL;

    std::vector<char> CopyMetadataToContiguous(
        const std::vector<MetaMetaInfoBlock> NewmetaMetaBlocks,
        const format::Buffer *MetaEncodeBuffer,
        const format::Buffer *AttributeEncodeBuffer, uint64_t DataSize,
        uint64_t WriterDataPos) const;

    std::vector<core::iovec> BreakoutContiguousMetadata(
        std::vector<char> *Aggregate, const std::vector<size_t> Counts,
        std::vector<MetaMetaInfoBlock> &UniqueMetaMetaBlocks,
        std::vector<core::iovec> &AttributeBlocks,
        std::vector<uint64_t> &DataSizes,
        std::vector<uint64_t> &WriterDataPositions) const;

    void *GetPtr(int bufferIdx, size_t posInBuffer);
    size_t CalcSize(const size_t Count, const size_t *Vals);

private:
    void Init();
    typedef struct _BP5WriterRec
    {
        void *Key;
        int FieldID;
        ShapeID Shape;
        size_t DataOffset;
        size_t MetaOffset;
        int DimCount;
        int Type;
    } * BP5WriterRec;

    struct FFSWriterMarshalBase
    {
        int RecCount = 0;
        BP5WriterRec RecList = NULL;
        FMContext LocalFMContext = {0};
        int MetaFieldCount = 0;
        FMFieldList MetaFields = NULL;
        FMFormat MetaFormat;
        int AttributeFieldCount = 0;
        FMFieldList AttributeFields = NULL;
        FMFormat AttributeFormat = NULL;
        void *AttributeData = NULL;
        int AttributeSize = 0;
    };

    struct DeferredExtern
    {
        size_t MetaOffset;
        size_t BlockID;
        const void *Data;
        size_t DataSize;
        size_t AlignReq;
    };
    std::vector<DeferredExtern> DeferredExterns;

    FFSWriterMarshalBase Info;
    void *MetadataBuf = NULL;
    bool NewAttribute = false;

    size_t MetadataSize = 0;
    BufferV *CurDataBuffer = NULL;
    std::vector<MetaMetaInfoBlock> PreviousMetaMetaInfoBlocks;

    size_t m_PriorDataBufferSizeTotal = 0;

    BP5WriterRec LookupWriterRec(void *Key);
    BP5WriterRec CreateWriterRec(void *Variable, const char *Name,
                                 DataType Type, size_t ElemSize,
                                 size_t DimCount);
    void RecalcMarshalStorageSize();
    void RecalcAttributeStorageSize();
    void AddSimpleField(FMFieldList *FieldP, int *CountP, const char *Name,
                        const char *Type, int ElementSize);
    void AddField(FMFieldList *FieldP, int *CountP, const char *Name,
                  const DataType Type, int ElementSize);
    void AddFixedArrayField(FMFieldList *FieldP, int *CountP, const char *Name,
                            const DataType Type, int ElementSize, int DimCount);
    void AddVarArrayField(FMFieldList *FieldP, int *CountP, const char *Name,
                          const DataType Type, int ElementSize,
                          char *SizeField);
    char *ConcatName(const char *base_name, const char *postfix,
                     ShapeID Shape = ShapeID::Unknown);
    const char *NamePrefix(ShapeID Shape);
    char *BuildVarName(const char *base_name, const int type,
                       const int element_size);
    void BreakdownVarName(const char *Name, char **base_name_p, int *type_p,
                          int *element_size_p);
    char *BuildArrayDimsName(const char *base_name, const int type,
                             const int element_size);
    char *BuildArrayDBCountName(const char *base_name, const int type,
                                const int element_size);
    char *BuildArrayBlockCountName(const char *base_name, const int type,
                                   const int element_size);
    char *TranslateADIOS2Type2FFS(const DataType Type);
    size_t *CopyDims(const size_t Count, const size_t *Vals);
    size_t *AppendDims(size_t *OldDims, const size_t OldCount,
                       const size_t Count, const size_t *Vals);

    void DumpDeferredBlocks();

    typedef struct _ArrayRec
    {
        size_t ElemCount;
        void *Array;
    } ArrayRec;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_B5_BP5Serializer_H_ */
