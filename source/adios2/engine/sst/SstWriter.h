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

    virtual ~SstWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void PerformPuts() final;
    void EndStep() final;

private:
    void Init(); ///< calls InitCapsules and InitTransports based on Method,
                 /// called from constructor

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &variable, const T *values) final;              \
    void DoPutDeferred(Variable<T> &, const T *) final;                        \
    void DoPutDeferred(Variable<T> &, const T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    SstStream m_Output;
    size_t m_WriterStep = -1;
    struct _SstParams Params;
#define declare_locals(Param, Type, Typedecl, Default)                         \
    Typedecl m_##Param = Default;
    SST_FOREACH_PARAMETER_TYPE_4ARGS(declare_locals);
#undef declare_locals

    void DoClose(const int transportIndex = -1) final;
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_SST_SST_WRITER_H_ */
