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

    typedef struct _TimestepInfo
    {
        std::vector<MetaMetaInfoBlock> NewMetaMetaBlocks;
        Buffer *MetaEncodeBuffer;
        Buffer *AttributeEncodeBuffer;
        BufferV *DataBuffer;
    } TimestepInfo;

    void Marshal(void *Variable, const char *Name, const DataType Type,
                 size_t ElemSize, size_t DimCount, const size_t *Shape,
                 const size_t *Count, const size_t *Offsets, const void *Data,
                 bool Sync);
    TimestepInfo CloseTimestep(int timestep);

private:
    void Init();
    typedef struct _FFSWriterRec
    {
        void *Key;
        int FieldID;
        size_t DataOffset;
        size_t MetaOffset;
        int DimCount;
        int Type;
    } * FFSWriterRec;

    struct FFSWriterMarshalBase
    {
        int RecCount = 0;
        FFSWriterRec RecList = NULL;
        FMContext LocalFMContext;
        int MetaFieldCount = 0;
        FMFieldList MetaFields = NULL;
        FMFormat MetaFormat;
        int DataFieldCount = 0;
        FMFieldList DataFields = NULL;
        FMFormat DataFormat;
        int AttributeFieldCount;
        FMFieldList AttributeFields;
        FMFormat AttributeFormat;
        void *AttributeData;
        int AttributeSize;
        int CompressZFP;
        attr_list ZFPParams;
    };

    FFSWriterMarshalBase Info;
    void *MetadataBuf = NULL;

    size_t MetadataSize = 0;
    BufferV *CurDataBuffer = NULL;
    std::vector<MetaMetaInfoBlock> PreviousMetaMetaInfoBlocks;

#ifdef NDEF
    typedef struct FFSVarRec
    {
        void *Variable;
        char *VarName;
        size_t *PerWriterMetaFieldOffset;
        size_t DimCount;
        int Type;
        int ElementSize;
        size_t *GlobalDims;
        size_t *PerWriterBlockStart;
        size_t *PerWriterBlockCount;
        size_t **PerWriterStart;
        size_t **PerWriterCounts;
        void **PerWriterIncomingData;
        size_t *PerWriterIncomingSize; // important for compression
    } * FFSVarRec;

    enum FFSRequestTypeEnum
    {
        Global = 0,
        Local = 1
    };

    typedef struct FFSArrayRequest
    {
        FFSVarRec VarRec;
        enum FFSRequestTypeEnum RequestType;
        size_t BlockID;
        size_t *Start;
        size_t *Count;
        void *Data;
        struct FFSArrayRequest *Next;
    } * FFSArrayRequest;
#endif
    FFSWriterRec LookupWriterRec(void *Key);
    FFSWriterRec CreateWriterRec(void *Variable, const char *Name,
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
    char *ConcatName(const char *base_name, const char *postfix);
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
    size_t CalcSize(const size_t Count, const size_t *Vals);

    typedef struct _ArrayRec
    {
        size_t ElemCount;
        void *Array;
    } ArrayRec;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_B5_BP5Serializer_H_ */
