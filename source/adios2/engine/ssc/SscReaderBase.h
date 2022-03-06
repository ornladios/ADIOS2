/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReaderBase.h
 *
 *  Created on: Mar 3, 2022
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCREADERBASE_H_
#define ADIOS2_ENGINE_SSCREADERBASE_H_

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

class SscReaderBase
{

public:
    SscReaderBase(IO &io, const std::string &name, const Mode mode,
                  MPI_Comm comm);
    virtual ~SscReaderBase();

    virtual StepStatus BeginStep(StepMode mode, const float timeoutSeconds) = 0;
    virtual size_t CurrentStep() = 0;
    virtual void PerformGets() = 0;
    virtual void EndStep() = 0;
    virtual void Close(const int transportIndex) = 0;

#define declare_type(T) virtual void GetDeferred(Variable<T> &, const T *) = 0;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
};

}
}
}
}

#endif // ADIOS2_ENGINE_SSCREADERBASE_H_
