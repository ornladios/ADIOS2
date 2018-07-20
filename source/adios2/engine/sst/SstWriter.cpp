/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Sst.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#include <memory>
#include <mpi.h>

#include "SstParamParser.h"
#include "SstWriter.h"
#include "SstWriter.tcc"

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
    m_WriterStep++;
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
        m_BP3Serializer->m_MetadataSet.TimeStep = m_WriterStep + 1;
        m_BP3Serializer->m_MetadataSet.CurrentStep = m_WriterStep;
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
    return StepStatus::OK;
}

void SstWriter::EndStep()
{
    if (m_MarshalMethod == SstMarshalFFS)
    {
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
        newblock->metadata.DataSize =
            m_BP3Serializer->m_Metadata.m_Buffer.size();
        newblock->metadata.block = m_BP3Serializer->m_Metadata.m_Buffer.data();
        newblock->data.DataSize = m_BP3Serializer->m_Data.m_Buffer.size();
        newblock->data.block = m_BP3Serializer->m_Data.m_Buffer.data();
        newblock->serializer = m_BP3Serializer;
        SstProvideTimestep(m_Output, &newblock->metadata, &newblock->data,
                           m_WriterStep, lf_FreeBlocks, newblock);
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
}

void SstWriter::PerformPuts() {}

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

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void SstWriter::DoClose(const int transportIndex) { SstWriterClose(m_Output); }

} // end namespace engine
} // end namespace core
} // end namespace adios2
