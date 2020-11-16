/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "SscWriter.tcc"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosCommMPI.h"
#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace core
{
namespace engine
{

SscWriter::SscWriter(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("SscWriter", io, name, mode, std::move(comm))
{
    TAU_SCOPED_TIMER_FUNC();

    helper::GetParameter(m_IO.m_Parameters, "MpiMode", m_MpiMode);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "OpenTimeoutSecs",
                         m_OpenTimeoutSecs);

    SyncMpiPattern();
    m_WriterRank = m_Comm.Rank();
    m_WriterSize = m_Comm.Size();
    MPI_Comm_rank(m_StreamComm, &m_StreamRank);
    MPI_Comm_size(m_StreamComm, &m_StreamSize);
}

StepStatus SscWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    ++m_CurrentStep;

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::BeginStep, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_WriterRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    if (m_CurrentStep == 0 || m_WriterDefinitionsLocked == false ||
        m_ReaderSelectionsLocked == false)
    {
        m_Buffer.resize(1, 0);
        m_GlobalWritePattern.clear();
        m_GlobalWritePattern.resize(m_StreamSize);
        m_GlobalReadPattern.clear();
        m_GlobalReadPattern.resize(m_StreamSize);
    }

    if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
    {
        if (m_CurrentStep > 1)
        {
            MpiWait();
        }
    }

    return StepStatus::OK;
}

size_t SscWriter::CurrentStep() const { return m_CurrentStep; }

void SscWriter::PerformPuts() { TAU_SCOPED_TIMER_FUNC(); }

void SscWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::EndStep, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_WriterRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    if (m_CurrentStep == 0)
    {
        SyncWritePattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                       m_StreamComm, &m_MpiWin);
        MPI_Win_free(&m_MpiWin);
        SyncReadPattern();
        if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
        {
            MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                           m_StreamComm, &m_MpiWin);
        }
    }
    else
    {
        if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
        {
            if (m_MpiMode == "twosided")
            {
                for (const auto &i : m_AllSendingReaderRanks)
                {
                    m_MpiRequests.emplace_back();
                    MPI_Isend(m_Buffer.data(),
                              static_cast<int>(m_Buffer.size()), MPI_CHAR,
                              i.first, 0, m_StreamComm, &m_MpiRequests.back());
                }
            }
            else if (m_MpiMode == "onesidedfencepush")
            {
                MPI_Win_fence(0, m_MpiWin);
                for (const auto &i : m_AllSendingReaderRanks)
                {
                    MPI_Put(m_Buffer.data(), static_cast<int>(m_Buffer.size()),
                            MPI_CHAR, i.first, i.second.first,
                            static_cast<int>(m_Buffer.size()), MPI_CHAR,
                            m_MpiWin);
                }
            }
            else if (m_MpiMode == "onesidedpostpush")
            {
                MPI_Win_start(m_MpiAllReadersGroup, 0, m_MpiWin);
                for (const auto &i : m_AllSendingReaderRanks)
                {
                    MPI_Put(m_Buffer.data(), static_cast<int>(m_Buffer.size()),
                            MPI_CHAR, i.first, i.second.first,
                            static_cast<int>(m_Buffer.size()), MPI_CHAR,
                            m_MpiWin);
                }
            }
            else if (m_MpiMode == "onesidedfencepull")
            {
                MPI_Win_fence(0, m_MpiWin);
            }
            else if (m_MpiMode == "onesidedpostpull")
            {
                MPI_Win_post(m_MpiAllReadersGroup, 0, m_MpiWin);
            }
        }
        else
        {
            SyncWritePattern();
            MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                           m_StreamComm, &m_MpiWin);
            MPI_Win_free(&m_MpiWin);
        }
    }
}

void SscWriter::Flush(const int transportIndex) { TAU_SCOPED_TIMER_FUNC(); }

// PRIVATE

void SscWriter::MpiWait()
{
    if (m_MpiMode == "twosided")
    {
        MPI_Waitall(static_cast<int>(m_MpiRequests.size()),
                    m_MpiRequests.data(), MPI_STATUSES_IGNORE);
        m_MpiRequests.clear();
    }
    else if (m_MpiMode == "onesidedfencepush")
    {
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "onesidedpostpush")
    {
        MPI_Win_complete(m_MpiWin);
    }
    else if (m_MpiMode == "onesidedfencepull")
    {
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "onesidedpostpull")
    {
        MPI_Win_wait(m_MpiWin);
    }
}

void SscWriter::SyncMpiPattern()
{
    TAU_SCOPED_TIMER_FUNC();

    MPI_Group streamGroup;
    MPI_Group writerGroup;
    MPI_Comm writerComm;
    MPI_Comm readerComm;

    helper::HandshakeComm(m_Name, 'w', m_OpenTimeoutSecs, CommAsMPI(m_Comm),
                          streamGroup, writerGroup, m_MpiAllReadersGroup,
                          m_StreamComm, writerComm, readerComm, m_Verbosity);
}

void SscWriter::SyncWritePattern(bool finalStep)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncWritePattern, World Rank " << m_StreamRank
                  << ", Writer Rank " << m_WriterRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    nlohmann::json localRankMetaJ;

    ssc::BlockVecToJson(m_GlobalWritePattern[m_StreamRank], localRankMetaJ);

    if (m_WriterRank == 0)
    {
        ssc::AttributeMapToJson(m_IO, localRankMetaJ);
        localRankMetaJ["Pattern"] = m_WriterDefinitionsLocked;
    }
    if (finalStep)
    {
        localRankMetaJ["FinalStep"] = true;
    }

    std::string localStr = localRankMetaJ.dump();

    // aggregate global write pattern
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  m_StreamComm);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.data(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_StreamSize, '\0');
    MPI_Allgather(localVec.data(), static_cast<int>(maxLocalSize), MPI_CHAR,
                  globalVec.data(), static_cast<int>(maxLocalSize), MPI_CHAR,
                  m_StreamComm);

    // deserialize global metadata Json
    nlohmann::json globalJson;
    ssc::LocalJsonToGlobalJson(globalVec, maxLocalSize, m_StreamSize,
                               globalJson);

    // deserialize variables metadata
    ssc::JsonToBlockVecVec(globalJson, m_GlobalWritePattern);

    if (m_Verbosity >= 10 && m_WriterRank == 0)
    {
        ssc::PrintBlockVecVec(m_GlobalWritePattern, "Global Write Pattern");
    }
}

void SscWriter::SyncReadPattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncReadPattern, World Rank " << m_StreamRank
                  << ", Writer Rank " << m_WriterRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    size_t localSize = 0;
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  m_StreamComm);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::vector<char> globalVec(maxLocalSize * m_StreamSize);
    MPI_Allgather(localVec.data(), static_cast<int>(maxLocalSize), MPI_CHAR,
                  globalVec.data(), static_cast<int>(maxLocalSize), MPI_CHAR,
                  m_StreamComm);

    // deserialize global metadata Json
    nlohmann::json globalJson;
    try
    {
        for (size_t i = 0; i < m_StreamSize; ++i)
        {
            if (globalVec[i * maxLocalSize] == '\0')
            {
                globalJson[i] = nullptr;
            }
            else
            {
                globalJson[i] = nlohmann::json::parse(
                    globalVec.begin() + i * maxLocalSize,
                    globalVec.begin() + (i + 1) * maxLocalSize);
            }
        }
    }
    catch (std::exception &e)
    {
        throw(std::runtime_error(
            std::string("corrupted global read pattern metadata, ") +
            std::string(e.what())));
    }

    ssc::JsonToBlockVecVec(globalJson, m_GlobalReadPattern);
    m_AllSendingReaderRanks = ssc::CalculateOverlap(
        m_GlobalReadPattern, m_GlobalWritePattern[m_StreamRank]);
    CalculatePosition(m_GlobalWritePattern, m_GlobalReadPattern, m_WriterRank,
                      m_AllSendingReaderRanks);

    for (int i = 0; i < m_StreamSize; ++i)
    {
        auto &patternJson = globalJson[i]["Pattern"];
        if (patternJson != nullptr)
        {
            m_ReaderSelectionsLocked = patternJson.get<bool>();
        }
    }
}

void SscWriter::CalculatePosition(ssc::BlockVecVec &writerVecVec,
                                  ssc::BlockVecVec &readerVecVec,
                                  const int writerRank,
                                  ssc::RankPosMap &allOverlapRanks)
{
    TAU_SCOPED_TIMER_FUNC();
    for (auto &overlapRank : allOverlapRanks)
    {
        auto &readerRankMap = readerVecVec[overlapRank.first];
        auto currentReaderOverlapWriterRanks =
            CalculateOverlap(writerVecVec, readerRankMap);
        size_t bufferPosition = 0;
        for (int rank = 0; rank < writerVecVec.size(); ++rank)
        {
            bool hasOverlap = false;
            for (const auto r : currentReaderOverlapWriterRanks)
            {
                if (r.first == rank)
                {
                    hasOverlap = true;
                    break;
                }
            }
            if (hasOverlap)
            {
                currentReaderOverlapWriterRanks[rank].first = bufferPosition;
                auto &bv = writerVecVec[rank];
                size_t currentRankTotalSize = TotalDataSize(bv) + 1;
                currentReaderOverlapWriterRanks[rank].second =
                    currentRankTotalSize;
                bufferPosition += currentRankTotalSize;
            }
        }
        allOverlapRanks[overlapRank.first] =
            currentReaderOverlapWriterRanks[writerRank];
    }
}

#define declare_type(T)                                                        \
    void SscWriter::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
        PerformPuts();                                                         \
    }                                                                          \
    void SscWriter::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SscWriter::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::DoClose, World Rank " << m_StreamRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    if (m_WriterDefinitionsLocked && m_ReaderSelectionsLocked)
    {
        if (m_CurrentStep > 0)
        {
            MpiWait();
        }

        m_Buffer[0] = 1;

        if (m_MpiMode == "twosided")
        {
            std::vector<MPI_Request> requests;
            for (const auto &i : m_AllSendingReaderRanks)
            {
                requests.emplace_back();
                MPI_Isend(m_Buffer.data(), 1, MPI_CHAR, i.first, 0,
                          m_StreamComm, &requests.back());
            }
            MPI_Waitall(static_cast<int>(requests.size()), requests.data(),
                        MPI_STATUS_IGNORE);
        }
        else if (m_MpiMode == "onesidedfencepush")
        {
            MPI_Win_fence(0, m_MpiWin);
            for (const auto &i : m_AllSendingReaderRanks)
            {
                MPI_Put(m_Buffer.data(), 1, MPI_CHAR, i.first, 0, 1, MPI_CHAR,
                        m_MpiWin);
            }
            MPI_Win_fence(0, m_MpiWin);
        }
        else if (m_MpiMode == "onesidedpostpush")
        {
            MPI_Win_start(m_MpiAllReadersGroup, 0, m_MpiWin);
            for (const auto &i : m_AllSendingReaderRanks)
            {
                MPI_Put(m_Buffer.data(), 1, MPI_CHAR, i.first, 0, 1, MPI_CHAR,
                        m_MpiWin);
            }
            MPI_Win_complete(m_MpiWin);
        }
        else if (m_MpiMode == "onesidedfencepull")
        {
            MPI_Win_fence(0, m_MpiWin);
            MPI_Win_fence(0, m_MpiWin);
        }
        else if (m_MpiMode == "onesidedpostpull")
        {
            MPI_Win_post(m_MpiAllReadersGroup, 0, m_MpiWin);
            MPI_Win_wait(m_MpiWin);
        }

        MPI_Win_free(&m_MpiWin);
    }
    else
    {
        SyncWritePattern(true);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
