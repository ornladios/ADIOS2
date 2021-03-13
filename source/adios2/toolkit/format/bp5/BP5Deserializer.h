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
    BP5Deserializer(int WriterCount);

    ~BP5Deserializer();

    void InstallMetaMetaData(MetaMetaInfoBlock &MMList);
    void InstallMetaData(void *MetadataBlock, size_t BlockLen,
                         size_t WriterRank);
    bool WriterIsRowMajor = 0;
    bool ReaderIsRowMajor = 0;
    core::Engine *m_Engine = NULL;

private:
    struct FFSVarRec
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
        FFSVarRec(int WriterSize)
        {
            PerWriterMetaFieldOffset.resize(WriterSize);
            PerWriterBlockStart.resize(WriterSize);
            PerWriterBlockCount.resize(WriterSize);
            PerWriterStart.resize(WriterSize);
            PerWriterCounts.resize(WriterSize);
            PerWriterIncomingData.resize(WriterSize);
            PerWriterIncomingSize.resize(WriterSize);
        }
    };

    struct ControlStruct
    {
        int FieldIndex;
        int FieldOffset;
        FFSVarRec *VarRec;
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

    FFSContext ReaderFFSContext;
    int WriterCohortSize;
    std::unordered_map<std::string, FFSVarRec *> VarByName;
    FMContext LocalFMContext;
    //    FFSArrayRequest PendingVarRequests;

    std::vector<void *> MetadataBaseAddrs;
    std::vector<FMFieldList> MetadataFieldLists;
    std::vector<void *> DataBaseAddrs;
    //  std::vector<FFSReaderPerWriterRec>WriterInfo;
    //  struct ControlInfo *ControlBlocks;

    ControlInfo *ControlBlocks = nullptr;
    ControlInfo *GetPriorControl(FMFormat Format);
    ControlInfo *BuildControl(FMFormat Format);
    bool NameIndicatesArray(const char *Name);
    DataType TranslateFFSType2ADIOS(const char *Type, int size);
    FFSVarRec *LookupVarByKey(void *Key);
    FFSVarRec *LookupVarByName(const char *Name);
    FFSVarRec *CreateVarRec(const char *ArrayName);
    void ReverseDimensions(size_t *Dimensions, int count);
    void BreakdownArrayName(const char *Name, char **base_name_p,
                            DataType *type_p, int *element_size_p);
    void *VarSetup(core::Engine *engine, const char *variableName,
                   const DataType type, void *data);
    void *ArrayVarSetup(core::Engine *engine, const char *variableName,
                        const DataType type, int DimCount, size_t *Shape,
                        size_t *Start, size_t *Count);
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP5_BP5Serializer_H_ */
