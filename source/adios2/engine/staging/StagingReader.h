/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingReader.h
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGREADER_H_
#define ADIOS2_ENGINE_STAGINGREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

class StagingReader : public Engine
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
    StagingReader(IO &adios, const std::string &name, const Mode mode,
                   MPI_Comm mpiComm);

    ~StagingReader();
    StepStatus BeginStep(
        StepMode mode = StepMode::NextAvailable,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

private:
    int m_Verbosity = 0;
    int m_ReaderRank; // my rank in the readers' comm

    // step info should be received from the writer side in BeginStep()
    int m_CurrentStep = -1;

    // EndStep must call PerformGets if necessary
    bool m_NeedPerformGets = false;

    void Init() final; ///< called from constructor, gets the selected Staging
                       /// transport method from settings
    void InitParameters() final;
    void InitTransports() final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1);

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGREADER_H_
