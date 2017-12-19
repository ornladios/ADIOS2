/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIReader.h
 * Class to exchange data using MPI between Writer and Reader
 *  partition of an application
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPIREADER_H_
#define ADIOS2_ENGINE_INSITUMPIREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

class InSituMPIReader : public Engine
{
public:
    /**
     * Constructor for single BP capsule engine, writes in BP format into a
     * single
     * heap capsule
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param hostLanguage
     */
    InSituMPIReader(IO &adios, const std::string &name, const Mode mode,
                    MPI_Comm mpiComm);

    ~InSituMPIReader();
    StepStatus BeginStep(StepMode mode = StepMode::NextAvailable,
                         const float timeoutSeconds = 0.f) final;
    void PerformGets() final;
    void EndStep() final;

    void Close(const int transportIndex = -1);

private:
    int m_Verbosity = 0;
    bool m_FixedSchedule = false; // true: metadata in steps does NOT change

    int m_GlobalRank; // my rank in the global comm
    int m_ReaderRank; // my rank in the readers' comm
    int m_GlobalNproc, m_ReaderNproc;

    // Global ranks of all the writers
    std::vector<int> m_RankAllPeers;
    // Global ranks of the writers directly assigned to me
    std::vector<int> m_RankDirectPeers;

    int m_CurrentStep = -1; // steps start from 0

    int m_NCallsPerformGets; // 1 and only 1 PerformGets() allowed per step
    int m_NDeferredGets;     // number of outstanding requests used in EndStep()

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    void DoGetDeferred(Variable<T> &, T &) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIMPIREADER_H_ */
