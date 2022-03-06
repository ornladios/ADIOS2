/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReaderGeneric.h
 *
 *  Created on: Mar 3, 2022
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCREADERGENERIC_H_
#define ADIOS2_ENGINE_SSCREADERGENERIC_H_

#include "SscReaderBase.h"
#include "adios2/core/IO.h"
#include <mpi.h>

namespace adios2
{
namespace core
{
namespace engine
{
namespace ssc
{

class SscReaderGeneric : SscReaderBase
{

public:
    SscReaderGeneric(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm comm);
    virtual ~SscReaderGeneric();

    virtual StepStatus BeginStep(StepMode mode,
                                 const float timeoutSeconds) final;
    virtual size_t CurrentStep() final;
    virtual void PerformGets() final;
    virtual void EndStep() final;
    virtual void Close(const int transportIndex) final;

#define declare_type(T)                                                        \
    virtual void GetDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
};

}
}
}
}

#endif // ADIOS2_ENGINE_SSCREADERGENERIC_H_
