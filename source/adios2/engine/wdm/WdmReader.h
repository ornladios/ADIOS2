/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WdmReader.h
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

#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/transportman/stagingman/StagingMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class WdmReader : public Engine
{
public:
    WdmReader(IO &adios, const std::string &name, const Mode mode,
              MPI_Comm mpiComm);

    ~WdmReader();
    StepStatus BeginStep(
        StepMode mode = StepMode::NextAvailable,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

private:
    bool m_Tolerance = true;
    format::DataManSerializer m_DataManSerializer;
    std::shared_ptr<transportman::StagingMan> m_DataTransport;
    std::shared_ptr<transportman::StagingMan> m_MetadataTransport;
    format::DmvVecPtrMap m_MetaDataMap;
    int64_t m_CurrentStep = -1;
    int m_MpiRank;
    std::vector<std::string> m_FullAddresses;
    int m_Timeout = 3;
    size_t m_ReaderId;
    std::string m_StepMode = "NextAvailable";

    struct Request
    {
        std::string variable;
        std::string type;
        size_t step;
        Dims start;
        Dims count;
        void *data;
    };
    std::vector<Request> m_DeferredRequests;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    void Handshake();
    template <typename T>
    void CheckIOVariable(const std::string &name, const Dims &shape,
                         const Dims &start, const Dims &count);

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1);

    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    void Log(const int level, const std::string &message, const bool mpi,
             const bool endline);

    int m_Verbosity = 0;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGREADER_H_
