/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4FileReader.h
 *
 *  Created on: Jul 31, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP4_BP4FILEREADER_H_
#define ADIOS2_ENGINE_BP4_BP4FILEREADER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp4/BP4.h" //format::BP1Deserializer
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{

namespace core
{
namespace engine
{

class BP4FileReader : public Engine
{

public:
    /**
     * Unique constructor
     * @param io
     * @param name
     * @param openMode only read
     * @param mpiComm
     */
    BP4FileReader(IO &io, const std::string &name, const Mode mode,
                 MPI_Comm mpiComm);

    virtual ~BP4FileReader() = default;

    StepStatus BeginStep(StepMode mode = StepMode::NextAvailable,
                         const float timeoutSeconds = 0.f) final;

    size_t CurrentStep() const final;

    void EndStep() final;

    void PerformGets() final;

private:
    format::BP4Deserializer m_BP4Deserializer;
    transportman::TransportMan m_FileManager;
    transportman::TransportMan m_SubFileManager;
    transportman::TransportMan m_FileMetadataIndexManager;

    /** used for per-step reads */
    size_t m_CurrentStep = 0;
    bool m_FirstStep = true;

    void Init();
    void InitTransports();
    void InitBuffer();

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    void
    ReadVariables(const std::map<std::string, helper::SubFileInfoMap> &variablesInfo);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4FILEREADER_H_ */
