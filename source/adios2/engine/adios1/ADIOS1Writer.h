/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS1Writer.h
 * Class to write files using old adios 1.x library.
 * It requires at least adios 1.12.0 installed
 *
 *  Created on: Mar 27, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_ADIOS1_ADIOS1WRITER_H_
#define ADIOS2_ENGINE_ADIOS1_ADIOS1WRITER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/interop/adios1/ADIOS1CommonWrite.h"

namespace adios2
{
namespace core
{
namespace engine
{

class ADIOS1Writer : public Engine
{

public:
    /**
     * Constructor for Writer writes in ADIOS 1.x BP format
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    ADIOS1Writer(IO &adios, const std::string &name, const Mode openMode,
                 MPI_Comm mpiComm);

    ~ADIOS1Writer() = default;

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void PerformPuts() final;
    void EndStep() final;

private:
    interop::ADIOS1CommonWrite m_ADIOS1;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &variable, const T *values) final;              \
    void DoPutDeferred(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports,
     * otherwise it closes a transport in m_Transport[transportIndex].
     * In debug mode the latter is bounds-checked.
     */
    void DoClose(const int transportIndex) final;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_ADIOS1_ADIOS1WRITER_H_ */
