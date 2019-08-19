/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReader.h
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCREADER_H_
#define ADIOS2_ENGINE_SSCREADER_H_

#include <queue>

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"
#include "adios2/toolkit/transportman/stagingman/StagingMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class SscReader : public Engine
{
public:
    SscReader(IO &adios, const std::string &name, const Mode mode,
              MPI_Comm mpiComm);

    ~SscReader();
    StepStatus BeginStep(
        StepMode stepMode = StepMode::Read,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    StepStatus BeginStepIterator(StepMode stepMode, format::DmvVecPtr &vars);
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

private:
    bool m_Tolerance = false;
    format::DataManSerializer m_DataManSerializer;
    std::shared_ptr<transportman::StagingMan> m_DataTransport;
    std::shared_ptr<transportman::StagingMan> m_MetadataTransport;
    format::DmvVecPtrMap m_MetaDataMap;
    int64_t m_CurrentStep = -1;
    int m_MpiRank;
    std::vector<std::string> m_FullAddresses;
    int m_Timeout = 3;
    int m_RetryMax = 128;
    size_t m_AppID;
    bool m_ConnectionLost = false;

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

    format::VecPtr m_RepliedMetadata;
    std::mutex m_RepliedMetadataMutex;

    void RequestMetadata(const int64_t step = -5);

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    template <typename T>
    void CheckIOVariable(const std::string &name, const Dims &shape);

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    DoAllStepsBlocksInfo(const Variable<T> &variable) const final;             \
    std::vector<typename Variable<T>::Info> DoBlocksInfo(                      \
        const Variable<T> &variable, const size_t step) const final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <typename T>
    std::map<size_t, std::vector<typename Variable<T>::Info>>
    AllStepsBlocksInfoCommon(const Variable<T> &variable) const;

    template <typename T>
    std::vector<typename Variable<T>::Info>
    BlocksInfoCommon(const Variable<T> &variable, const size_t step) const;

    template <typename T>
    void AccumulateMinMax(T &min, T &max, const std::vector<char> &minVec,
                          const std::vector<char> &maxVec) const;

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

#endif // ADIOS2_ENGINE_SSCREADER_H_
