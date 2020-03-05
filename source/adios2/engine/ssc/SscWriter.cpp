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
#include "adios2/helper/adiosJSONcomplex.h"
#include "nlohmann/json.hpp"

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
    MPI_Comm_rank(MPI_COMM_WORLD, &m_WorldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_WorldSize);
    m_WriterRank = m_Comm.Rank();
    m_WriterSize = m_Comm.Size();

    ssc::GetParameter(m_IO.m_Parameters, "MpiMode", m_MpiMode);
    ssc::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    ssc::GetParameter(m_IO.m_Parameters, "MaxFilenameLength",
                      m_MaxFilenameLength);
    ssc::GetParameter(m_IO.m_Parameters, "RendezvousAppCount",
                      m_RendezvousAppCount);
    ssc::GetParameter(m_IO.m_Parameters, "RendezvousStreamCount",
                      m_RendezvousStreamCount);

    m_GlobalWritePattern.resize(m_WorldSize);
    m_GlobalReadPattern.resize(m_WorldSize);
    m_Buffer.resize(1);
    SyncMpiPattern();
}

StepStatus SscWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_InitialStep)
    {
        m_InitialStep = false;
    }
    else
    {
        ++m_CurrentStep;
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::BeginStep, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t SscWriter::CurrentStep() const
{
    TAU_SCOPED_TIMER_FUNC();
    return m_CurrentStep;
}

void SscWriter::PerformPuts() { TAU_SCOPED_TIMER_FUNC(); }

void SscWriter::PutOneSidedPostPush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_start(m_MpiAllReadersGroup, 0, m_MpiWin);
    for (const auto &i : m_AllSendingReaderRanks)
    {
        MPI_Put(m_Buffer.data(), m_Buffer.size(), MPI_CHAR, i.first,
                i.second.first, m_Buffer.size(), MPI_CHAR, m_MpiWin);
    }
    MPI_Win_complete(m_MpiWin);
}

void SscWriter::PutOneSidedFencePush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_fence(0, m_MpiWin);
    for (const auto &i : m_AllSendingReaderRanks)
    {
        MPI_Put(m_Buffer.data(), m_Buffer.size(), MPI_CHAR, i.first,
                i.second.first, m_Buffer.size(), MPI_CHAR, m_MpiWin);
    }
    MPI_Win_fence(0, m_MpiWin);
}

void SscWriter::PutOneSidedPostPull()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_post(m_MpiAllReadersGroup, 0, m_MpiWin);
    MPI_Win_wait(m_MpiWin);
}

void SscWriter::PutOneSidedFencePull()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_fence(0, m_MpiWin);
    MPI_Win_fence(0, m_MpiWin);
}

void SscWriter::PutTwoSided()
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<MPI_Request> requests;
    for (const auto &i : m_AllSendingReaderRanks)
    {
        requests.emplace_back();
        MPI_Isend(m_Buffer.data(), m_Buffer.size(), MPI_CHAR, i.first, 0,
                  MPI_COMM_WORLD, &requests.back());
    }
    MPI_Status statuses[requests.size()];
    MPI_Waitall(requests.size(), requests.data(), statuses);
}

void SscWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::EndStep, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    if (m_CurrentStep == 0)
    {
        SyncWritePattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                       MPI_COMM_WORLD, &m_MpiWin);
        PutOneSidedPostPull();
        MPI_Win_free(&m_MpiWin);
        SyncReadPattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                       MPI_COMM_WORLD, &m_MpiWin);
    }
    else
    {
        if (m_MpiMode == "TwoSided")
        {
            PutTwoSided();
        }
        else if (m_MpiMode == "OneSidedFencePush")
        {
            PutOneSidedFencePush();
        }
        else if (m_MpiMode == "OneSidedPostPush")
        {
            PutOneSidedPostPush();
        }
        else if (m_MpiMode == "OneSidedFencePull")
        {
            PutOneSidedFencePull();
        }
        else if (m_MpiMode == "OneSidedPostPull")
        {
            PutOneSidedPostPull();
        }
    }
}

void SscWriter::Flush(const int transportIndex) { TAU_SCOPED_TIMER_FUNC(); }

// PRIVATE

void SscWriter::SyncMpiPattern()
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncMpiPattern, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    m_MpiHandshake.Start(m_RendezvousStreamCount, m_MaxFilenameLength,
                         m_RendezvousAppCount, 'w', m_Name, CommAsMPI(m_Comm));
    m_MpiHandshake.Wait(m_Name);
    m_MpiHandshake.PrintMaps();

    for (const auto &app : m_MpiHandshake.GetWriterMap(m_Name))
    {
        for (int rank : app.second)
        {
            m_AllWriterRanks.push_back(rank);
        }
    }

    for (const auto &app : m_MpiHandshake.GetReaderMap(m_Name))
    {
        for (int rank : app.second)
        {
            m_AllReaderRanks.push_back(rank);
        }
    }

    m_MpiHandshake.Finalize();

    MPI_Group worldGroup;
    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    MPI_Group_incl(worldGroup, m_AllReaderRanks.size(), m_AllReaderRanks.data(),
                   &m_MpiAllReadersGroup);
}

void SscWriter::SyncWritePattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncWritePattern, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    // serialize local writer rank variables metadata
    nlohmann::json localRankMetaJ;
    for (const auto &b : m_GlobalWritePattern[m_WorldRank])
    {
        localRankMetaJ["Variables"].emplace_back();
        auto &jref = localRankMetaJ["Variables"].back();
        jref["Name"] = b.name;
        jref["Type"] = b.type;
        jref["Shape"] = b.shape;
        jref["Start"] = b.start;
        jref["Count"] = b.count;
        jref["BufferStart"] = b.bufferStart;
        jref["BufferCount"] = b.bufferCount;
    }

    // serialize local writer rank attributes metadata
    auto &attributesJson = localRankMetaJ["Attributes"];
    const auto &attributeMap = m_IO.GetAttributesDataMap();
    for (const auto &attributePair : attributeMap)
    {
        const std::string name(attributePair.first);
        const std::string type(attributePair.second.first);
        if (type.empty())
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        const auto &attribute = m_IO.InquireAttribute<T>(name);                \
        nlohmann::json attributeJson;                                          \
        attributeJson["Name"] = attribute->m_Name;                             \
        attributeJson["Type"] = attribute->m_Type;                             \
        attributeJson["IsSingleValue"] = attribute->m_IsSingleValue;           \
        if (attribute->m_IsSingleValue)                                        \
        {                                                                      \
            attributeJson["Value"] = attribute->m_DataSingleValue;             \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            attributeJson["Array"] = attribute->m_DataArray;                   \
        }                                                                      \
        attributesJson.emplace_back(std::move(attributeJson));                 \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    std::string localStr = localRankMetaJ.dump();

    // aggregate global write pattern
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  MPI_COMM_WORLD);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.data(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_WorldSize, '\0');
    MPI_Allgather(localVec.data(), maxLocalSize, MPI_CHAR, globalVec.data(),
                  maxLocalSize, MPI_CHAR, MPI_COMM_WORLD);

    // deserialize global metadata Json
    nlohmann::json globalJson;
    try
    {
        for (size_t i = 0; i < m_WorldSize; ++i)
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
            std::string("corrupted global write pattern metadata, ") +
            std::string(e.what())));
    }

    // deserialize variables metadata
    ssc::JsonToBlockVecVec(globalJson, m_GlobalWritePattern);
}

void SscWriter::SyncReadPattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncReadPattern, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    size_t localSize = 0;
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  MPI_COMM_WORLD);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::vector<char> globalVec(maxLocalSize * m_WorldSize);
    MPI_Allgather(localVec.data(), maxLocalSize, MPI_CHAR, globalVec.data(),
                  maxLocalSize, MPI_CHAR, MPI_COMM_WORLD);

    // deserialize global metadata Json
    nlohmann::json globalJson;
    try
    {
        for (size_t i = 0; i < m_WorldSize; ++i)
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
    ssc::CalculateOverlap(m_GlobalReadPattern,
                          m_GlobalWritePattern[m_WorldRank]);
    m_AllSendingReaderRanks = ssc::AllOverlapRanks(m_GlobalReadPattern);
    CalculatePosition(m_GlobalWritePattern, m_GlobalReadPattern, m_WriterRank,
                      m_AllSendingReaderRanks);

    if (m_Verbosity >= 10)
    {
        ssc::PrintBlockVecVec(m_GlobalWritePattern, "Global Write Pattern");
        ssc::PrintBlockVec(m_GlobalWritePattern[m_WriterRank],
                           "Local Write Pattern");
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
        CalculateOverlap(writerVecVec, readerRankMap);
        auto currentReaderOverlapWriterRanks = AllOverlapRanks(writerVecVec);
        size_t bufferPosition = 0;
        for (size_t rank = 0; rank < writerVecVec.size(); ++rank)
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
        std::cout << "SscWriter::DoClose, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    m_Buffer[0] = 1;

    if (m_MpiMode == "TwoSided")
    {
        std::vector<MPI_Request> requests;
        for (const auto &i : m_AllSendingReaderRanks)
        {
            requests.emplace_back();
            MPI_Isend(m_Buffer.data(), 1, MPI_CHAR, i.first, 0, MPI_COMM_WORLD,
                      &requests.back());
        }
        MPI_Status statuses[requests.size()];
        MPI_Waitall(requests.size(), requests.data(), statuses);
    }
    else if (m_MpiMode == "OneSidedFencePush")
    {
        MPI_Win_fence(0, m_MpiWin);
        for (const auto &i : m_AllSendingReaderRanks)
        {
            MPI_Put(m_Buffer.data(), 1, MPI_CHAR, i.first, 0, 1, MPI_CHAR,
                    m_MpiWin);
        }
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "OneSidedPostPush")
    {
        MPI_Win_start(m_MpiAllReadersGroup, 0, m_MpiWin);
        for (const auto &i : m_AllSendingReaderRanks)
        {
            MPI_Put(m_Buffer.data(), 1, MPI_CHAR, i.first, 0, 1, MPI_CHAR,
                    m_MpiWin);
        }
        MPI_Win_complete(m_MpiWin);
    }
    else if (m_MpiMode == "OneSidedFencePull")
    {
        MPI_Win_fence(0, m_MpiWin);
        MPI_Win_fence(0, m_MpiWin);
    }
    else if (m_MpiMode == "OneSidedPostPull")
    {
        MPI_Win_post(m_MpiAllReadersGroup, 0, m_MpiWin);
        MPI_Win_wait(m_MpiWin);
    }

    MPI_Win_free(&m_MpiWin);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
