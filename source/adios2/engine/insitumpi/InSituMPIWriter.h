/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIWriter.h
 * Class to exchange data using MPI between Writer and Reader
 *  partition of an application
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPIMPIWRITER_H_
#define ADIOS2_ENGINE_INSITUMPIMPIWRITER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"

namespace adios2
{

class InSituMPIWriter : public Engine
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
    InSituMPIWriter(IO &adios, const std::string &name, const Mode openMode,
                    MPI_Comm mpiComm);

    ~InSituMPIWriter() = default;

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void PerformPuts() final;
    void EndStep() final;

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports, otherwise
     * it
     * closes a transport in m_Transport[transportIndex]. In debug mode the
     * latter
     * is bounds-checked.
     */
    void Close(const int transportIndex = -1) final;

private:
    int m_Verbosity = 0;
    bool m_FixedSchedule = false; // true: metadata in steps does NOT change
    MPI_Comm m_WriterComm;
    int m_GlobalRank; // my rank in the global comm
    int m_WriterRank; // my rank in the writers' comm
    int m_GlobalNproc, m_WriterNproc, m_ReaderNproc;
    std::vector<int> m_ReaderPeers; // Global ranks of the writers

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;                        \
    void DoPutDeferred(Variable<T> &, const T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

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

#endif /* ADIOS2_ENGINE_INSITUMPIMPIWRITER_H_ */
