/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Sst.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include "adios2/helper/adiosComm.h"
#include <memory>

#include "SstParamParser.h"
#include "SstWriter.h"
#include "SstWriter.tcc"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#else
#include "adios2/toolkit/sst/mpiwrap.h"
#endif

namespace adios2
{
namespace core
{
namespace engine
{

SstWriter::SstWriter(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("SstWriter", io, name, mode, std::move(comm))
{
    auto AssembleMetadata = [](void *writer, int CohortSize,
                               struct _SstData * /*PerRankMetadata*/,
                               struct _SstData * /*PerRankAttributeData*/) {
        for (int i = 0; i < CohortSize; i++)
        {
            //            std::cout << "Rank " << i << " Metadata size is "
            //                      << PerRankMetadata[i].DataSize << ", base
            //                      address is "
            //                      << static_cast<void
            //                      *>(PerRankMetadata[i].block)
            //                      << std::endl;
        }
        /*
         *  This lambda function is provided to SST, and subsequently called
         *  from inside SstProvideTimestep on rank 0, but the
         *  PerRankMetadata and PerRankAttributeData entries correspond with
         *  the individual Metadata and AttributeData blocks that were
         *  passed to SstProvideTimestep by each writer rank.  That is, SST
         *  has brought those independent blocks to rank 0, and the purpose
         *  of this function is to assemble them into a single block of
         *  aggregate metadata.
         *
         *  This function should:
         *   - Take the PerRankMetadata blocks (which are just the Metadata
         * values that were passed to SstProvideTimestep) and assemble them into
         * a single serialized block
         *   - PerRankMetadata[0] should be modified to hold the address and
         * size of this single serialized block
         *   - If this single serialized block creates objects/data that must be
         * free'd later, return some value (cast to void*) that will enable you
         * to free it when it is passed to FreeAssembleMetadata.  (You'll also
         * get back PerRankMetadata[0], so if that's sufficient, you can return
         * NULL here.)
         *
         *  This function should *not* free any of the blocks pointer to by
         * PerRankMetadata.  It need not zero out or otherwise modify the
         * PerRankMetadata entries other than entry [0];

         *
         *  Note that this code works as is with current BP3 marshalling
         *  because it's already the case that only PerRanksMetadata[0] is
         *  valid (the rest are Null/0), and only 0 is used on the reading
         *  side.  This is the case because
         *  m_BP3Serializer->AggregateCollectiveMetadata() serializes the
         *  individual contributions of each rank, gathers them to rank 0,
         *  assembles them into a single block and then passes that to
         *  SstProvideTimestep().  However, this approach costs us extra MPI
         *  operations because AggregateCollectiveMetadata is doing them,
         *  and then we have more to do inside SstProvideTimestep().  To
         *  avoid these multiple MPI ops, we need to *not* use
         *  AggregateCollectiveMetadata.  Instead, each rank should
         *  serialize its individual contribution, and then pass that to
         *  ProvideTimestep.  Then this function should be changed to do the
         *  "assemble them into a single block" part.  Assuming that the
         *  result is the same as what AggregateCollectiveMetadata would
         *  have done, then the reader side should be able to remain
         *  unchanged.
         *
         *  Note also that this code is capable of handling marshaling
         *  systems that provide the attributes separately from the timestep
         *  metadata.  This is necessary in order to maintain attribute
         *  semantics when timesteps might be dropped on the writer side
         *  because of queue limits, or to be provided to late arriving
         *  readers, etc.  BP marshaling does not yet provide the ability to
         *  marshal attributes separately (AFAIK), so the AttributeData
         *  params can be ignored.
         */
        return (void *)malloc(1); /* return value will be passed as ClientData
                                     to registered Free routine */
    };

    auto FreeAssembledMetadata =
        [](void * /*writer*/, struct _SstData * /*PerRankMetadata*/,
           struct _SstData * /*PerRankAttributeData*/, void *ClientData) {
            //        std::cout << "Free called with client data " << ClientData
            //        << std::endl;
            free(ClientData);
            return;
        };

    Init();

    m_Output = SstWriterOpen(name.c_str(), &Params,
#ifdef ADIOS2_HAVE_MPI
                             CommAsMPI(m_Comm)
#else
                             MPI_COMM_NULL
#endif
    );

    if (m_MarshalMethod == SstMarshalBP)
    {
        SstWriterInitMetadataCallback(m_Output, this, AssembleMetadata,
                                      FreeAssembledMetadata);
    }
}

SstWriter::~SstWriter() { SstStreamDestroy(m_Output); }

StepStatus SstWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    TAU_SCOPED_TIMER_FUNC();
    m_WriterStep++;
    if (m_BetweenStepPairs)
    {
        throw std::logic_error("ERROR: BeginStep() is called a second time "
                               "without an intervening EndStep()");
    }

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
        m_BP3Serializer = new format::BP3Serializer(m_Comm, m_DebugMode);
        m_BP3Serializer->Init(m_IO.m_Parameters,
                              "in call to BP3::Open for writing");
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
            m_Comm, m_BP3Serializer->m_Metadata, true);
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
