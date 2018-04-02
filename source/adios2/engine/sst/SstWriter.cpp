/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Sst.cpp
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

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
        // Do whatever might be necessary to initiate BP marshalling, maybe
        // allocating buffers or creating BP objects
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
        SstData Blocks = new _SstData[2];
        SstData MetaDataBlock = &Blocks[0];
        SstData DataBlock = &Blocks[1];

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

        auto lf_FreeBlocks = [](void *vBlocks) {
            SstData BlocksToFree = reinterpret_cast<SstData>(vBlocks);
            //  Free data and metadata blocks here.  BlocksToFree is the Blocks
            //  value in the enclosing function.
            delete BlocksToFree;
        };

        MetaDataBlock->DataSize = /* set size of metadata block */ 0;
        MetaDataBlock->block = /* set to address of metadata block */ NULL;
        DataBlock->DataSize = /* set size of data block */ 0;
        DataBlock->block = /* set to address of data block */ NULL;
        SstProvideTimestep(m_Output, MetaDataBlock, DataBlock, m_WriterStep,
                           lf_FreeBlocks, Blocks);
        // DON'T FREE YOUR DATA OR METADATA BLOCKS HERE.  See lf_FreeBlocks.
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
