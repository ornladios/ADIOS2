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

#ifndef ADIOS2_ENGINE_INSITUMPI_INSITUMPIWRITER_H_
#define ADIOS2_ENGINE_INSITUMPI_INSITUMPIWRITER_H_

#include "InSituMPISchedules.h"
#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/format/bp/bp3/BP3Serializer.h"

namespace adios2
{
namespace core
{
namespace engine
{

class InSituMPIWriter : public Engine
{

public:
    /**
     * Constructor for Writer writes in ADIOS 1.x BP format
     * @param name unique name given to the engine
     * @param accessMode
     * @param comm
     * @param method
     * @param debugMode
     */
    InSituMPIWriter(IO &adios, const std::string &name, const Mode openMode,
                    helper::Comm comm);

    ~InSituMPIWriter();

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
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
    // Mapping from global ranks to peer IDs
    std::map<int, int> m_RankToPeerID;
    // Global ranks of the readers directly assigned to me
    std::vector<int> m_RankDirectPeers;
    // true: Reader(s) selected me as primary writer contact
    // e.g. I need to send info to Readers in BeginStep()
    bool m_AmIPrimaryContact;

    int m_CurrentStep = -1; // steps start from 0

    int m_NCallsPerformPuts; // 1 and only 1 PerformPuts() allowed per step

    /** Single object controlling BP buffering used only for metadata in this
     * engine */
    format::BP3Serializer m_BP3Serializer;

    /** Collection of all read requests. Created at first PerformPuts().
     * Recreated at every PerformPuts() if do not have fixed schedule.
     */
    insitumpi::WriteScheduleMap m_WriteScheduleMap;

    /** true: We know that the source/targe has a fixed write/read schedule
     * and this engine can utilize this fact for optimizing I/O
     */
    bool m_RemoteDefinitionsLocked = false;

    std::vector<MPI_Request> m_MPIRequests; // for MPI_Waitall in EndStep()

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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
     * @param blockInfo current block info
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable,
                       const typename Variable<T>::Info &blockInfo);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    /** Send data asynchronously of a variable to all readers that
     * has requested a piece
     * void AsyncSendVariable(const std::string &varName);
     */
    template <class T>
    void AsyncSendVariable(Variable<T> &variable,
                           const typename Variable<T>::Info &blockInfo);

    void AsyncSendVariable(std::string variableName);

    /**
     * Receive read schedule from readers and build write schedule
     */
    void ReceiveReadSchedule(insitumpi::WriteScheduleMap &writeScheduleMap);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPI_INSITUMPIWRITER_H_ */
