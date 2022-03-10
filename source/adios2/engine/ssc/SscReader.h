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

#include "SscHelper.h"
#include "SscReaderBase.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosMpiHandshake.h"
#include <mpi.h>
#include <queue>
#include <unordered_set>

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
              helper::Comm comm);
    virtual ~SscReader();
    StepStatus BeginStep(
        StepMode stepMode = StepMode::Read,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    void PerformGets() final;
    size_t CurrentStep() const final;
    void EndStep() final;

private:
    int64_t m_CurrentStep = -1;
    bool m_StepBegun = false;

    ssc::BlockVecVec m_GlobalWritePattern;
    ssc::BlockVec m_LocalReadPattern;
    ssc::Buffer m_GlobalWritePatternBuffer;

    ssc::RankPosMap m_AllReceivingWriterRanks;
    ssc::Buffer m_Buffer;
    MPI_Win m_MpiWin;
    MPI_Group m_WriterGroup;
    MPI_Comm m_StreamComm;
    MPI_Comm m_ReaderComm;
    std::vector<MPI_Request> m_MpiRequests;
    StepStatus m_StepStatus;
    std::thread m_EndStepThread;

    int m_StreamRank;
    int m_StreamSize;
    int m_ReaderRank;
    int m_ReaderSize;
    int m_WriterMasterStreamRank;
    int m_ReaderMasterStreamRank;

    void SyncMpiPattern();
    bool SyncWritePattern();
    void SyncReadPattern();
    void BeginStepConsequentFixed();
    void BeginStepFlexible(StepStatus &status);
    void EndStepFixed();
    void EndStepFirstFlexible();
    void EndStepConsequentFlexible();

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    std::vector<typename Variable<T>::BPInfo> DoBlocksInfo(                    \
        const Variable<T> &variable, const size_t step) const final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <typename T>
    std::vector<typename Variable<T>::BPInfo>
    BlocksInfoCommon(const Variable<T> &variable, const size_t step) const;

    void DoClose(const int transportIndex = -1);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredDeltaCommon(Variable<T> &variable, T *data);

    void CalculatePosition(ssc::BlockVecVec &mapVec,
                           ssc::RankPosMap &allOverlapRanks);

    int m_OpenTimeoutSecs = 10;
    bool m_Threading = false;

    int m_Verbosity = 0;
    std::string m_EngineMode = "generic";
    std::shared_ptr<ssc::SscReaderBase> m_EngineInstance;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCREADER_H_
