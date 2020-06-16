/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.h
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_SSCWRITER_H_
#define ADIOS2_ENGINE_SSCWRITER_H_

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

class SscWriter : public Engine
{

public:
    SscWriter(IO &adios, const std::string &name, const Mode mode,
              helper::Comm comm);
    ~SscWriter() = default;

    StepStatus BeginStep(
        StepMode mode,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    size_t m_CurrentStep = 0;
    bool m_InitialStep = true;

    ssc::BlockVecVec m_GlobalWritePattern;
    ssc::BlockVecVec m_GlobalReadPattern;

    ssc::RankPosMap m_AllSendingReaderRanks;
    std::vector<char> m_Buffer;
    MPI_Win m_MpiWin;
    MPI_Group m_MpiAllReadersGroup;
    MPI_Comm m_StreamComm;
    std::string m_MpiMode = "twosided";
    bool m_NeedWait = false;
    std::vector<MPI_Request> m_MpiRequests;

    int m_StreamRank;
    int m_StreamSize;
    int m_WriterRank;
    int m_WriterSize;

    helper::MpiHandshake m_MpiHandshake;

    void SyncMpiPattern();
    void SyncWritePattern();
    void SyncReadPattern();
    void PutOneSidedFencePush();
    void PutOneSidedPostPush();
    void PutOneSidedFencePull();
    void PutOneSidedPostPull();
    void PutTwoSided();
    void MpiWait();

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    void CalculatePosition(ssc::BlockVecVec &writerMapVec,
                           ssc::BlockVecVec &readerMapVec, const int writerRank,
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

#endif // ADIOS2_ENGINE_SSCWRITER_H_
