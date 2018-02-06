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

#include <iostream> //needs to go away, this is just for demo purposes

namespace adios2
{

SstWriter::SstWriter(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("SstWriter", io, name, mode, mpiComm)
{
    char *cstr = new char[name.length() + 1];
    strcpy(cstr, name.c_str());

    Init();
#define set_params(Param, Type, Typedecl, Default) Params.Param = m_##Param;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(set_params);
#undef set_params

    m_Output = SstWriterOpen(cstr, &Params, mpiComm);
    delete[] cstr;
}

StepStatus SstWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    if (m_FFSmarshal)
    {
        return (StepStatus)SstWriterBeginStep(m_Output, (int)mode, timeout_sec);
    }
    else
    {
        // When BP marshalling/unmarshaling complete, this should call
        // SstProvideTimestep and clean up at this level
    }
    return StepStatus::OK;
}

void SstWriter::EndStep()
{
    if (m_FFSmarshal)
    {
        SstWriterEndStep(m_Output);
    }
    else
    {
        // When BP marshalling/unmarshaling complete, this should call
        // SstProvideTimestep and clean up at this level
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

#define get_params(Param, Type, Typedecl, Default)                             \
    lf_Set##Type##Parameter("##Param##", m_##Param);
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
