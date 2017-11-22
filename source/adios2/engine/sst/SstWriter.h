/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SstWriter.h
 *
 *  Created on: Aug 17, 2017
 *      Author: Greg Eisenhauer
 */

#ifndef ADIOS2_ENGINE_SST_SST_WRITER_H_
#define ADIOS2_ENGINE_SST_SST_WRITER_H_

#include <iostream> //std::cout must be removed, only used for hello example
#include <unistd.h> //sleep must be removed

#include <mpi.h>

#include "adios2/toolkit/sst/sst.h"

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

namespace adios2
{

class SstWriter : public Engine
{

public:
    SstWriter(IO &io, const std::string &name, const Mode mode,
              MPI_Comm mpiComm);

    virtual ~SstWriter() = default;

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void EndStep() final;

    void Close(const int transportIndex = -1) final;

private:
    void Init(); ///< calls InitCapsules and InitTransports based on Method,
                 /// called from constructor

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    SstStream m_Output;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_SST_SST_WRITER_H_ */
