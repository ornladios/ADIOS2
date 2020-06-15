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
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosMpiHandshake.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"
#include <mpi.h>
#include <queue>

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
    size_t m_CurrentStep = 0;
    bool m_InitialStep = true;

    ssc::BlockVecVec m_GlobalWritePattern;
    ssc::BlockVec m_LocalReadPattern;
    nlohmann::json m_GlobalWritePatternJson;
    nlohmann::json m_LocalReadPatternJson;

    ssc::RankPosMap m_AllReceivingWriterRanks;
    std::vector<char> m_Buffer;
    MPI_Win m_MpiWin;
    MPI_Group m_MpiAllWritersGroup;
    MPI_Comm m_StreamComm;
    std::string m_MpiMode = "twosided";
    std::vector<MPI_Request> m_MpiRequests;

    int m_StreamRank;
    int m_StreamSize;
    int m_ReaderRank;
    int m_ReaderSize;

    helper::MpiHandshake m_MpiHandshake;

    void SyncMpiPattern();
    void SyncWritePattern();
    void SyncReadPattern();
    void GetOneSidedFencePush();
    void GetOneSidedPostPush();
    void GetOneSidedFencePull();
    void GetOneSidedPostPull();
    void GetTwoSided();

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;                              \
    std::vector<typename Variable<T>::Info> DoBlocksInfo(                      \
        const Variable<T> &variable, const size_t step) const final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <typename T>
    std::vector<typename Variable<T>::Info>
    BlocksInfoCommon(const Variable<T> &variable, const size_t step) const;

    void DoClose(const int transportIndex = -1);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);

    void CalculatePosition(ssc::BlockVecVec &mapVec,
                           ssc::RankPosMap &allOverlapRanks);

    int m_Verbosity = 0;
    int m_MaxFilenameLength = 128;
    int m_MaxStreamsPerApp = 1;
    int m_RendezvousAppCount = 2;
    int m_OpenTimeoutSecs = 10;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_SSCREADER_H_
