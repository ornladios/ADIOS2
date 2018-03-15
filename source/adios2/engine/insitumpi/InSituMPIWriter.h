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

#include "InSituMPISchedules.h"
#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h"

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

    ~InSituMPIWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void PerformPuts() final;
    void EndStep() final;

private:
    MPI_Comm m_CommWorld = MPI_COMM_WORLD;
    int m_Verbosity = 0;
    int m_GlobalRank; // my rank in the global comm
    int m_WriterRank; // my rank in the writers' comm
    int m_GlobalNproc, m_WriterNproc;

    // Global ranks of all the readers
    std::vector<int> m_RankAllPeers;
    // Global ranks of the readers directly assigned to me
    std::vector<int> m_RankDirectPeers;

    int m_CurrentStep = -1; // steps start from 0

    int m_NCallsPerformPuts; // 1 and only 1 PerformPuts() allowed per step

    /** Single object controlling BP buffering used only for metadata in this
     * engine */
    format::BP3Serializer m_BP3Serializer;

    /** Collection of all read requests. Created at first PerformPuts().
     * Recreated at every PerformPuts() if do not have fixed schedule.
     */
    insitumpi::WriteScheduleMap m_WriteScheduleMap;

    std::vector<MPI_Request> m_MPIRequests; // for MPI_Waitall in EndStep()

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

    /** Send data asynchronously of a variable to all readers that
     * has requested a piece
     * void AsyncSendVariable(const std::string &varName);
     */
    template <class T>
    void AsyncSendVariable(Variable<T> &);

    void AsyncSendVariable(std::string variableName);
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIMPIWRITER_H_ */
