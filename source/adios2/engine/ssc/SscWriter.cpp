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
    m_ReaderSize = m_WorldSize - m_WriterSize;

    m_GlobalWritePattern.resize(m_WriterSize);
    m_GlobalReadPattern.resize(m_ReaderSize);

    SyncMpiPattern();
}

StepStatus SscWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::BeginStep, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    if (m_InitialStep)
    {
        m_InitialStep = false;
    }
    else
    {
        ++m_CurrentStep;
    }
    return StepStatus::OK;
}

size_t SscWriter::CurrentStep() const
{
    TAU_SCOPED_TIMER_FUNC();
    return m_CurrentStep;
}

void SscWriter::PerformPuts() { TAU_SCOPED_TIMER_FUNC(); }

void SscWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::EndStep, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    if (m_CurrentStep == 0)
    {
        SyncWritePattern();
        SyncReadPattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), sizeof(char),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &m_MpiWin);
    }

    MPI_Win_fence(0, m_MpiWin);
    for (const auto &i : m_AllSendingReaderRanks)
    {
        MPI_Put(m_Buffer.data(), m_Buffer.size(), MPI_CHAR,
                m_ReaderMasterWorldRank + i.first, i.second.first + 1,
                m_Buffer.size(), MPI_CHAR, m_MpiWin);
    }
    MPI_Win_fence(0, m_MpiWin);
}

void SscWriter::Flush(const int transportIndex) { TAU_SCOPED_TIMER_FUNC(); }

// PRIVATE

void SscWriter::SyncMpiPattern()
{
    int readerMasterWorldRank = 0;
    int writerMasterWorldRank = 0;
    if (m_WriterRank == 0)
    {
        writerMasterWorldRank = m_WorldRank;
    }
    MPI_Allreduce(&readerMasterWorldRank, &m_ReaderMasterWorldRank, 1, MPI_INT,
                  MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&writerMasterWorldRank, &m_WriterMasterWorldRank, 1, MPI_INT,
                  MPI_MAX, MPI_COMM_WORLD);

    if (m_WorldSize == m_WriterSize)
    {
        throw(std::runtime_error("no readers are found"));
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncMpiPattern, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }
}

void SscWriter::SyncWritePattern()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncWritePattern, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    // serialize local writer rank metadata
    nlohmann::json j;
    size_t blockIndex = 0;
    size_t position = 0;

    for (const auto &b : m_GlobalWritePattern[m_WriterRank])
    {
        j.emplace_back();
        auto &jref = j.back();
        jref["N"] = b.name;
        jref["T"] = b.type;
        jref["S"] = b.shape;
        jref["O"] = b.start;
        jref["C"] = b.count;
        jref["X"] = b.bufferStart;
        jref["Y"] = b.bufferCount;
    }
    std::string localStr = j.dump();

    // aggregate global metadata across all writers
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    m_Comm.Allreduce(&localSize, &maxLocalSize, 1, helper::Comm::Op::Max);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.data(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_WriterSize);
    m_Comm.GatherArrays(localVec.data(), maxLocalSize, globalVec.data(), 0);

    std::string globalStr;
    if (m_WriterRank == 0)
    {
        nlohmann::json globalJson;
        try
        {
            for (size_t i = 0; i < m_WriterSize; ++i)
            {
                globalJson[i] = nlohmann::json::parse(
                    globalVec.begin() + i * maxLocalSize,
                    globalVec.begin() + (i + 1) * maxLocalSize);
            }
        }
        catch (...)
        {
            throw(std::runtime_error(
                "writer received corrupted aggregated metadata"));
        }

        nlohmann::json attributesJson;
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
        attributeJson["N"] = attribute->m_Name;                                \
        attributeJson["T"] = attribute->m_Type;                                \
        attributeJson["S"] = attribute->m_IsSingleValue;                       \
        if (attribute->m_IsSingleValue)                                        \
        {                                                                      \
            attributeJson["V"] = attribute->m_DataSingleValue;                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            attributeJson["V"] = attribute->m_DataArray;                       \
        }                                                                      \
        attributesJson.emplace_back(std::move(attributeJson));                 \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
        }
        globalJson[m_WriterSize] = attributesJson;
        globalStr = globalJson.dump();
    }

    size_t globalSizeSrc = globalStr.size();
    size_t globalSizeDst = 0;
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG,
                  MPI_MAX, MPI_COMM_WORLD);

    if (globalStr.size() < globalSizeDst)
    {
        globalStr.resize(globalSizeDst);
    }

    globalVec.resize(globalSizeDst);
    std::memcpy(globalVec.data(), globalStr.data(), globalStr.size());

    // sync with readers
    MPI_Win win;
    MPI_Win_create(globalVec.data(), globalVec.size(), sizeof(char),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    if (m_WorldRank > 0)
    {
        MPI_Get(globalVec.data(), globalVec.size(), MPI_CHAR, 0, 0,
                globalVec.size(), MPI_CHAR, win);
    }
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    ssc::JsonToBlockVecVec(globalVec, m_GlobalWritePattern);

    m_Buffer.resize(ssc::TotalDataSize(m_GlobalWritePattern[m_WriterRank]));
}

void SscWriter::SyncReadPattern()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "SscWriter::SyncReadPattern, World Rank " << m_WorldRank
                  << ", Writer Rank " << m_WriterRank << std::endl;
    }

    size_t globalSizeSrc = 0;
    size_t globalSizeDst = 0;
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG,
                  MPI_MAX, MPI_COMM_WORLD);

    std::vector<char> globalVec(globalSizeDst);

    MPI_Win win;
    MPI_Win_create(globalVec.data(), globalVec.size(), sizeof(char),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Get(globalVec.data(), globalVec.size(), MPI_CHAR,
            m_ReaderMasterWorldRank, 0, globalVec.size(), MPI_CHAR, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    ssc::JsonToBlockVecVec(globalVec, m_GlobalReadPattern);
    ssc::CalculateOverlap(m_GlobalReadPattern,
                          m_GlobalWritePattern[m_WriterRank]);
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
                size_t currentRankTotalSize = TotalDataSize(bv);
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

    MPI_Win_fence(0, m_MpiWin);
    if (m_WriterRank == 0)
    {
        m_Buffer[0] = 1;
        for (int i = 0; i < m_ReaderSize; ++i)
        {
            MPI_Put(m_Buffer.data(), 1, MPI_CHAR, m_ReaderMasterWorldRank + i,
                    0, 1, MPI_CHAR, m_MpiWin);
        }
    }
    MPI_Win_fence(0, m_MpiWin);
    MPI_Win_free(&m_MpiWin);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
