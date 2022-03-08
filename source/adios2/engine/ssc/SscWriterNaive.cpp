/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriterNaive.cpp
 *
 *  Created on: Mar 7, 2022
 *      Author: Jason Wang
 */

#include "SscWriterNaive.tcc"

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

SscWriterNaive::SscWriterNaive(IO &io, const std::string &name, const Mode mode,
                               MPI_Comm comm)
: SscWriterBase(io, name, mode, comm)
{
}

StepStatus SscWriterNaive::BeginStep(const StepMode mode,
                                     const float timeoutSeconds,
                                     const bool writerLocked)
{
    ++m_CurrentStep;
    return StepStatus::OK;
}

size_t SscWriterNaive::CurrentStep() { return m_CurrentStep; }

void SscWriterNaive::PerformPuts() {}

void SscWriterNaive::EndStep(const bool writerLocked) {}

void SscWriterNaive::Close(const int transportIndex) {}

#define declare_type(T)                                                        \
    void SscWriterNaive::PutDeferred(Variable<T> &variable, const T *data)     \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

}
}
}
}
