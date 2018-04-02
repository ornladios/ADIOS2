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

#include "SstWriter.h"
#include "SstWriter.tcc"

namespace adios2
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
    if (m_FFSmarshal)
    {
        return (StepStatus)SstFFSWriterBeginStep(m_Output, (int)mode,
                                                 timeout_sec);
    }
    else if (m_BPmarshal)
    {
        // initialize BP serializer, deleted in
        // SstWriter::EndStep()::lf_FreeBlocks()
        m_BP3Serializer = new format::BP3Serializer(m_MPIComm, m_DebugMode);
        m_BP3Serializer->InitParameters(m_IO.m_Parameters);
    }
    else
    {
        // unknown marshaling method, shouldn't happen
    }
    return StepStatus::OK;
}

void SstWriter::EndStep()
{
    if (m_FFSmarshal)
    {
        SstFFSWriterEndStep(m_Output, m_WriterStep);
    }
    else if (m_BPmarshal)
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
        m_BP3Serializer->AggregateCollectiveMetadata();

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
    auto lf_SetBoolParameter = [&](const std::string key, int &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            if (itKey->second == "yes" || itKey->second == "true")
            {
                parameter = 1;
            }
            else if (itKey->second == "no" || itKey->second == "false")
            {
                parameter = 0;
            }
        }
    };
    auto lf_SetIntParameter = [&](const std::string key, int &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            parameter = std::stoi(itKey->second);
            return true;
        }
        return false;
    };

    auto lf_SetStringParameter = [&](const std::string key, char *&parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            parameter = strdup(itKey->second.c_str());
            return true;
        }
        return false;
    };

    auto lf_SetRegMethodParameter = [&](const std::string key,
                                        size_t &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            std::string method = itKey->second;
            std::transform(method.begin(), method.end(), method.begin(),
                           ::tolower);
            if (method == "file")
            {
                parameter = SstRegisterFile;
            }
            else if (method == "screen")
            {
                parameter = SstRegisterScreen;
            }
            else if (method == "cloud")
            {
                parameter = SstRegisterCloud;
                throw std::invalid_argument("ERROR: Sst RegistrationMethod "
                                            "\"cloud\" not yet implemented" +
                                            m_EndMessage);
            }
            else
            {
                throw std::invalid_argument(
                    "ERROR: Unknown Sst RegistrationMethod parameter \"" +
                    method + "\"" + m_EndMessage);
            }
            return true;
        }
        return false;
    };

#define get_params(Param, Type, Typedecl, Default)                             \
    lf_Set##Type##Parameter(#Param, m_##Param);
    SST_FOREACH_PARAMETER_TYPE_4ARGS(get_params);
#undef get_params
#define set_params(Param, Type, Typedecl, Default) Params.Param = m_##Param;
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
    }                                                                          \
    void SstWriter::DoPutDeferred(Variable<T> &, const T &value) {}
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void SstWriter::DoClose(const int transportIndex) { SstWriterClose(m_Output); }

} // end namespace adios2
