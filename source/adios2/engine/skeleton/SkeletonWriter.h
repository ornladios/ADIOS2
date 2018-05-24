/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SkeletonWriter.h
 * Skeleton engine from which any engine can be built.
 *
 *  Created on: Jan 04, 2018
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_SKELETONMPIWRITER_H_
#define ADIOS2_ENGINE_SKELETONMPIWRITER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

namespace adios2
{

class SkeletonWriter : public Engine
{

public:
    /**
     * Constructor for Writer
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    SkeletonWriter(IO &adios, const std::string &name, const Mode mode,
                   MPI_Comm mpiComm);

    ~SkeletonWriter() = default;

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void PerformPuts() final;
    void EndStep() final;

private:
    int m_Verbosity = 0;
    int m_WriterRank;       // my rank in the writers' comm
    int m_CurrentStep = -1; // steps start from 0

    // EndStep must call PerformPuts if necessary
    bool m_NeedPerformPuts = false;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports,
     * otherwise it closes a transport in m_Transport[transportIndex].
     * In debug mode the latter is bounds-checked.
     */
    void DoClose(const int transportIndex = -1) final;

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_SKELETONMPIWRITER_H_ */
