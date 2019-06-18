/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Sst.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include <adios2/common/ADIOSMPI.h>
#include <memory>

#include "SstParamParser.h"
#include "SstWriter.h"
#include "SstWriter.tcc"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

namespace adios2
{
namespace core
{
namespace engine
{

SstWriter::SstWriter(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("SstWriter", io, name, mode, mpiComm)
{
    char *cstr = new char[name.length() + 1];
    strcpy(cstr, name.c_str());

    Init();

    m_Output = SstWriterOpen(cstr, &Params, mpiComm);
    delete[] cstr;
}

SstWriter::~SstWriter() { SstStreamDestroy(m_Output); }

StepStatus SstWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    TAU_SCOPED_TIMER_FUNC();
    m_WriterStep++;
    m_BetweenStepPairs = true;
    if (m_MarshalMethod == SstMarshalFFS)
    {
        return (StepStatus)SstFFSWriterBeginStep(m_Output, (int)mode,
                                                 timeout_sec);
    }
    else if (m_MarshalMethod == SstMarshalBP)
    {
        // initialize BP serializer, deleted in
        // SstWriter::EndStep()::lf_FreeBlocks()
        m_BP3Serializer = new format::BP3Serializer(m_MPIComm, m_DebugMode);
        m_BP3Serializer->InitParameters(m_IO.m_Parameters);
        m_BP3Serializer->m_MetadataSet.TimeStep = 1;
        m_BP3Serializer->m_MetadataSet.CurrentStep = m_WriterStep;
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
    return StepStatus::OK;
}

void SstWriter::FFSMarshalAttributes()
{
    TAU_SCOPED_TIMER_FUNC();
    const auto &attributesDataMap = m_IO.GetAttributesDataMap();

    const uint32_t attributesCount =
        static_cast<uint32_t>(attributesDataMap.size());

    // if there are no new attributes, nothing to do
    if (attributesCount == m_FFSMarshaledAttributesCount)
        return;

    for (const auto &attributePair : attributesDataMap)
    {
        const std::string name(attributePair.first);
        const std::string type(attributePair.second.first);

        if (type == "unknown")
        {
        }
        else if (type == helper::GetType<std::string>())
        {
            core::Attribute<std::string> &attribute =
                *m_IO.InquireAttribute<std::string>(name);
            int element_count = -1;
            const char *data_addr = attribute.m_DataSingleValue.c_str();
            if (!attribute.m_IsSingleValue)
            {
                //
            }

            SstFFSMarshalAttribute(m_Output, name.c_str(), type.c_str(),
                                   sizeof(char *), element_count, data_addr);
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Attribute<T> &attribute = *m_IO.InquireAttribute<T>(name);       \
        int element_count = -1;                                                \
        void *data_addr = &attribute.m_DataSingleValue;                        \
        if (!attribute.m_IsSingleValue)                                        \
        {                                                                      \
            element_count = attribute.m_Elements;                              \
            data_addr = attribute.m_DataArray.data();                          \
        }                                                                      \
        SstFFSMarshalAttribute(m_Output, attribute.m_Name.c_str(),             \
                               type.c_str(), sizeof(T), element_count,         \
                               data_addr);                                     \
    }

        ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void SstWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    m_BetweenStepPairs = false;
    if (m_WriterDefinitionsLocked && !m_DefinitionsNotified)
    {
        SstWriterDefinitionLock(m_Output, m_WriterStep);
        m_DefinitionsNotified = true;
    }
    if (m_MarshalMethod == SstMarshalFFS)
    {
        TAU_SCOPED_TIMER("Marshaling Overhead");
        TAU_START("SstMarshalFFS");
        FFSMarshalAttributes();
        TAU_STOP("SstMarshalFFS");
        SstFFSWriterEndStep(m_Output, m_WriterStep);
    }
    else if (m_MarshalMethod == SstMarshalBP)
    {
        // This should finalize BP marshaling at the writer side.  All
        // marshaling methods should result in two blocks, one a block of
        // local metadata, and the second a block of data.  The metadata
        // will be aggregated across all writer ranks (by SST, inside
        // SstProvideTimestep) and made available to the readers (as a set
        // of N metadata blocks).  The Data block will be held on the writer
        // side (inside SstProvideTimestep), waiting on read requests from
        // individual reader ranks.  Any metadata or data blocks created
        // here should not be deallocated when SstProvideTimestep returns!
        // They should not be deallocated until SST is done with them
        // (explicit deallocation callback).
        TAU_START("Marshaling overhead");
        auto lf_FreeBlocks = [](void *vBlock) {
            BP3DataBlock *BlockToFree =
                reinterpret_cast<BP3DataBlock *>(vBlock);
            //  Free data and metadata blocks here.  BlockToFree is the newblock
            //  value in the enclosing function.
            delete BlockToFree->serializer;
            delete BlockToFree;
        };

        m_BP3Serializer->CloseStream(m_IO, true);
        m_BP3Serializer->AggregateCollectiveMetadata(
            m_MPIComm, m_BP3Serializer->m_Metadata, true);
        BP3DataBlock *newblock = new BP3DataBlock;
        newblock->metadata.DataSize = m_BP3Serializer->m_Metadata.m_Position;
        newblock->metadata.block = m_BP3Serializer->m_Metadata.m_Buffer.data();
        newblock->data.DataSize = m_BP3Serializer->m_Data.m_Position;
        newblock->data.block = m_BP3Serializer->m_Data.m_Buffer.data();
        newblock->serializer = m_BP3Serializer;
        TAU_STOP("Marshaling overhead");
        SstProvideTimestep(m_Output, &newblock->metadata, &newblock->data,
                           m_WriterStep, lf_FreeBlocks, newblock, NULL, NULL,
                           NULL);
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
}

void SstWriter::PerformPuts() {}

void SstWriter::Flush(const int transportIndex) {}

// PRIVATE functions below
void SstWriter::Init()
{
    SstParamParser Parser;

    Parser.ParseParams(m_IO, Params);

#define set_params(Param, Type, Typedecl, Default) m_##Param = Params.Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(set_params);
#undef set_params
}

#define declare_type(T)                                                        \
    void SstWriter::DoPutSync(Variable<T> &variable, const T *values)          \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void SstWriter::DoPutDeferred(Variable<T> &variable, const T *values)      \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SstWriter::DoClose(const int transportIndex) { SstWriterClose(m_Output); }

} // end namespace engine
} // end namespace core
} // end namespace adios2
