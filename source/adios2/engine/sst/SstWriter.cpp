/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adios2/helper/adiosComm.h"
#include <memory>

#include "SstParamParser.h"
#include "SstWriter.h"
#include "SstWriter.tcc"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include <adios2-perfstubs-interface.h>

namespace adios2
{
namespace core
{
namespace engine
{

SstWriter::SstWriter(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("SstWriter", io, name, mode, std::move(comm))
{
    Init();

    m_Output = SstWriterOpen(name.c_str(), &Params, &m_Comm);
    m_IsOpen = true;
}

SstWriter::~SstWriter()
{
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    SstStreamDestroy(m_Output);
}

StepStatus SstWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    m_WriterStep++;
    if (m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "SstWriter", "BeginStep",
                                        "BeginStep() is called a second time "
                                        "without an intervening EndStep()");
    }

    m_BetweenStepPairs = true;
    m_BP5Serializer.InitStep(new format::MallocV("SstWriter", true));
    return StepStatus::OK;
}

size_t SstWriter::CurrentStep() const { return m_WriterStep; }

void SstWriter::MarshalAttributes(std::list<std::vector<const char *>> &stringArrayData)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    const auto &attributes = m_IO.GetAttributes();

    if ((m_WriterStep == 0) && Params.UseOneTimeAttributes)
    {
        for (const auto &attributePair : attributes)
        {
            m_BP5Serializer.OnetimeMarshalAttribute(*(attributePair.second));
        }
        m_MarshalAttributesNecessary = false;
    }

    // if there are no new attributes, nothing to do
    if (!m_MarshalAttributesNecessary)
        return;

    for (const auto &attributePair : attributes)
    {
        const std::string name(attributePair.first);
        const DataType type(attributePair.second->m_Type);

        if (type == DataType::None)
        {
        }
        else if (type == helper::GetDataType<std::string>())
        {
            core::Attribute<std::string> &attribute = *m_IO.InquireAttribute<std::string>(name);
            if (attribute.m_IsSingleValue)
            {
                const char *data_addr = attribute.m_DataSingleValue.c_str();
                m_BP5Serializer.MarshalAttribute(name.c_str(), type, sizeof(char *), -1, data_addr);
            }
            else
            {
                // array of strings, marshaled as an array of char pointers
                const int element_count = (int)attribute.m_Elements;
                stringArrayData.emplace_back(element_count);
                auto &strings = stringArrayData.back();
                for (int i = 0; i < element_count; i++)
                {
                    strings[i] = attribute.m_DataArray[i].c_str();
                }
                m_BP5Serializer.MarshalAttribute(name.c_str(), type, sizeof(char *), element_count,
                                                 strings.data());
            }
        }
#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        core::Attribute<T> &attribute = *m_IO.InquireAttribute<T>(name);                           \
        int element_count = -1;                                                                    \
        void *data_addr = &attribute.m_DataSingleValue;                                            \
        if (!attribute.m_IsSingleValue)                                                            \
        {                                                                                          \
            element_count = (int)attribute.m_Elements;                                             \
            data_addr = attribute.m_DataArray.data();                                              \
        }                                                                                          \
        m_BP5Serializer.MarshalAttribute(attribute.m_Name.c_str(), type, sizeof(T), element_count, \
                                         data_addr);                                               \
    }

        ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
    m_MarshalAttributesNecessary = false;
}

void SstWriter::NotifyEngineAttribute(std::string name, DataType type) noexcept
{
    helper::Throw<std::invalid_argument>("SstWriter", "Engine", "ThrowUp",
                                         "Engine does not support NotifyEngineAttribute");
}

void SstWriter::NotifyEngineAttribute(std::string name, AttributeBase *Attr, void *data) noexcept
{
    if (!Params.UseOneTimeAttributes)
    {
        m_MarshalAttributesNecessary = true;
        return;
    }

    m_BP5Serializer.OnetimeMarshalAttribute(*Attr);
    m_MarshalAttributesNecessary = false;
}

void SstWriter::EndStep()
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    if (!m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "SstWriter", "EndStep",
                                        "EndStep() is called without a successful BeginStep()");
    }
    m_BetweenStepPairs = false;
    if (m_WriterDefinitionsLocked && !m_DefinitionsNotified)
    {
        SstWriterDefinitionLock(m_Output, m_WriterStep);
        m_DefinitionsNotified = true;
    }
    // The serializer keeps pointers to the char* arrays of string array
    // attributes and encodes them in CloseTimestep(), so they must live
    // until then.
    std::list<std::vector<const char *>> stringArrayData;
    MarshalAttributes(stringArrayData);
    format::BP5Serializer::TimestepInfo *TSInfo =
        new format::BP5Serializer::TimestepInfo(m_BP5Serializer.CloseTimestep(m_WriterStep));
    auto lf_FreeBlocks = [](void *vBlock) {
        BP5DataBlock *BlockToFree = reinterpret_cast<BP5DataBlock *>(vBlock);
        //  Free data and metadata blocks here.  BlockToFree is the newblock
        //  value in the enclosing function.
        free(BlockToFree->MetaMetaBlocks);
        delete BlockToFree->TSInfo->DataBuffer;
        delete BlockToFree->TSInfo;
        delete BlockToFree;
    };

    BP5DataBlock *newblock = new BP5DataBlock;
    SstMetaMetaList MetaMetaBlocks =
        (SstMetaMetaList)malloc((TSInfo->NewMetaMetaBlocks.size() + 1) * sizeof(MetaMetaBlocks[0]));
    int i = 0;
    for (const auto &MM : TSInfo->NewMetaMetaBlocks)
    {
        MetaMetaBlocks[i].BlockData = MM.MetaMetaInfo;
        MetaMetaBlocks[i].BlockSize = MM.MetaMetaInfoLen;
        MetaMetaBlocks[i].ID = MM.MetaMetaID;
        MetaMetaBlocks[i].IDSize = MM.MetaMetaIDLen;
        i++;
    }
    MetaMetaBlocks[TSInfo->NewMetaMetaBlocks.size()] = {NULL, 0, NULL, 0};
    newblock->MetaMetaBlocks = MetaMetaBlocks;
    newblock->metadata.DataSize = TSInfo->MetaEncodeBuffer->m_FixedSize;
    newblock->metadata.block = TSInfo->MetaEncodeBuffer->Data();
    std::vector<core::iovec> iovec = TSInfo->DataBuffer->DataVec();
    if (!iovec.empty())
    {
        newblock->data.DataSize = iovec[0].iov_len;
        newblock->data.block = (char *)iovec[0].iov_base;
    }
    else
    {
        newblock->data.DataSize = 0;
        newblock->data.block = nullptr;
    }
    newblock->TSInfo = TSInfo;
    if (TSInfo->AttributeEncodeBuffer)
    {
        newblock->attribute_data.DataSize = TSInfo->AttributeEncodeBuffer->m_FixedSize;
        newblock->attribute_data.block = TSInfo->AttributeEncodeBuffer->Data();
    }
    else
    {
        newblock->attribute_data.DataSize = 0;
        newblock->attribute_data.block = NULL;
    }
    SstProvideTimestepMM(m_Output, &newblock->metadata, &newblock->data, m_WriterStep,
                         lf_FreeBlocks, newblock, &newblock->attribute_data, NULL, newblock,
                         MetaMetaBlocks);
}

void SstWriter::PerformPuts() {}

void SstWriter::Flush(const int transportIndex) {}

// PRIVATE functions below
void SstWriter::Init()
{
    SstParamParser Parser;

    Parser.ParseParams(m_IO, Params, m_UserOptions);

    if (Params.verbose < 0 || Params.verbose > 5)
    {
        helper::Throw<std::invalid_argument>("Engine", "SstWriter", "Init",
                                             "ERROR: Method verbose argument must be an "
                                             "integer in the range [0,5], in call to "
                                             "Open or Engine constructor\n");
    }
    m_BP5Serializer.m_StatsLevel = Params.StatsLevel;
    m_BP5Serializer.m_Engine = this;
}

#define declare_type(T)                                                                            \
    void SstWriter::DoPutSync(Variable<T> &variable, const T *values)                              \
    {                                                                                              \
        PutSyncCommon(variable, values);                                                           \
    }                                                                                              \
    void SstWriter::DoPutDeferred(Variable<T> &variable, const T *values)                          \
    {                                                                                              \
        PutSyncCommon(variable, values);                                                           \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SstWriter::PutStructCommon(VariableBase &variable, const void *data)
{
    size_t *Shape = NULL;
    size_t *Start = NULL;
    size_t *Count = NULL;
    size_t DimCount = 0;

    if (m_BetweenStepPairs == false)
    {
        helper::Throw<std::logic_error>("Engine", "SstWriter", "PutSyncCommon",
                                        "When using the SST engine in ADIOS2, "
                                        "Put() calls must appear between "
                                        "BeginStep/EndStep pairs");
    }

    if (variable.m_ShapeID == ShapeID::GlobalArray)
    {
        DimCount = variable.m_Shape.size();
        Shape = variable.m_Shape.data();
        Start = variable.m_Start.data();
        Count = variable.m_Count.data();
    }
    else if (variable.m_ShapeID == ShapeID::LocalArray)
    {
        DimCount = variable.m_Count.size();
        Count = variable.m_Count.data();
    }
    m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(), variable.m_Type,
                            variable.m_ElementSize, DimCount, Shape, Count, Start, data, true,
                            nullptr);
}

void SstWriter::DoPutStructSync(VariableStruct &variable, const void *data)
{
    PutStructCommon(variable, data);
}

void SstWriter::DoPutStructDeferred(VariableStruct &variable, const void *data)
{
    PutStructCommon(variable, data);
}

void PutStruct(VariableStruct &, const void *, bool);

void SstWriter::DoClose(const int transportIndex) { SstWriterClose(m_Output); }

/**
 * Called if destructor is called on an open engine.  Should warn or take any
 * non-complex measure that might help recover.
 */
void SstWriter::DestructorClose(bool Verbose) noexcept
{
    if (Verbose)
    {
        std::cerr << "SST Writer \"" << m_Name << "\" Destroyed without a prior Close()."
                  << std::endl;
        std::cerr << "This may result in loss of data and/or disconnect "
                     "warnings for a connected SST Reader."
                  << std::endl;
    }
    m_IsOpen = false;
    // should at least call control plane to remove contact file
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
