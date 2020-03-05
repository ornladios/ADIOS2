/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReader.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "SscReader.tcc"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosCommMPI.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosJSONcomplex.h"
#include "nlohmann/json.hpp"

namespace adios2
{
namespace core
{
namespace engine
{

SscReader::SscReader(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("SscReader", io, name, mode, std::move(comm))
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Comm_rank(MPI_COMM_WORLD, &m_WorldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_WorldSize);
    m_ReaderRank = m_Comm.Rank();
    m_ReaderSize = m_Comm.Size();

    ssc::GetParameter(m_IO.m_Parameters, "MpiMode", m_MpiMode);
    ssc::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    ssc::GetParameter(m_IO.m_Parameters, "MaxFilenameLength",
                      m_MaxFilenameLength);
    ssc::GetParameter(m_IO.m_Parameters, "RendezvousAppCount",
                      m_RendezvousAppCount);
    ssc::GetParameter(m_IO.m_Parameters, "RendezvousStreamCount",
                      m_RendezvousStreamCount);

    m_Buffer.resize(1);

    m_GlobalWritePattern.resize(m_WorldSize);
    SyncMpiPattern();
    SyncWritePattern();
}

SscReader::~SscReader() { TAU_SCOPED_TIMER_FUNC(); }

void SscReader::GetOneSidedPostPush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_post(m_MpiAllWritersGroup, 0, m_MpiWin);
    MPI_Win_wait(m_MpiWin);
}

void SscReader::GetOneSidedFencePush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_fence(0, m_MpiWin);
    MPI_Win_fence(0, m_MpiWin);
}

void SscReader::GetOneSidedPostPull()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_start(m_MpiAllWritersGroup, 0, m_MpiWin);
    for (const auto &i : m_AllReceivingWriterRanks)
    {
        MPI_Get(m_Buffer.data() + i.second.first, i.second.second, MPI_CHAR,
                i.first, 0, i.second.second, MPI_CHAR, m_MpiWin);
    }
    MPI_Win_complete(m_MpiWin);
}

void SscReader::GetOneSidedFencePull()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_fence(0, m_MpiWin);
    for (const auto &i : m_AllReceivingWriterRanks)
    {
        MPI_Get(m_Buffer.data() + i.second.first, i.second.second, MPI_CHAR,
                i.first, 0, i.second.second, MPI_CHAR, m_MpiWin);
    }
    MPI_Win_fence(0, m_MpiWin);
}

void SscReader::GetTwoSided()
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<MPI_Request> requests;
    for (const auto &i : m_AllReceivingWriterRanks)
    {
        requests.emplace_back();
        MPI_Irecv(m_Buffer.data() + i.second.first, i.second.second, MPI_CHAR,
                  i.first, 0, MPI_COMM_WORLD, &requests.back());
    }
    MPI_Status statuses[requests.size()];
    MPI_Waitall(requests.size(), requests.data(), statuses);
}

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_InitialStep)
    {
        m_InitialStep = false;
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &m_MpiWin);
        MPI_Win_start(m_MpiAllWritersGroup, 0, m_MpiWin);
    }
    else
    {
        ++m_CurrentStep;
        if (m_MpiMode == "TwoSided")
        {
            GetTwoSided();
        }
        else if (m_MpiMode == "OneSidedFencePush")
        {
            GetOneSidedFencePush();
        }
        else if (m_MpiMode == "OneSidedPostPush")
        {
            GetOneSidedPostPush();
        }
        else if (m_MpiMode == "OneSidedFencePull")
        {
            GetOneSidedFencePull();
        }
        else if (m_MpiMode == "OneSidedPostPull")
        {
            GetOneSidedPostPull();
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::BeginStep, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << ", Step "
                  << m_CurrentStep << std::endl;
    }

    if (m_Buffer[0] == 1)
    {
        return StepStatus::EndOfStream;
    }

    return StepStatus::OK;
}

void SscReader::PerformGets() { TAU_SCOPED_TIMER_FUNC(); }

size_t SscReader::CurrentStep() const
{
    TAU_SCOPED_TIMER_FUNC();
    return m_CurrentStep;
}

void SscReader::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_CurrentStep == 0)
    {
        MPI_Win_complete(m_MpiWin);
        MPI_Win_free(&m_MpiWin);
        SyncReadPattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                       MPI_COMM_WORLD, &m_MpiWin);
    }
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::EndStep, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }
}

// PRIVATE

void SscReader::SyncMpiPattern()
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncMpiPattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    m_MpiHandshake.Start(m_RendezvousStreamCount, m_MaxFilenameLength,
                         m_RendezvousAppCount, 'r', m_Name, CommAsMPI(m_Comm));
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
    MPI_Group_incl(worldGroup, m_AllWriterRanks.size(), m_AllWriterRanks.data(),
                   &m_MpiAllWritersGroup);
}

void SscReader::SyncWritePattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncWritePattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    // aggregate global write pattern
    size_t localSize = 0;
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  MPI_COMM_WORLD);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::vector<char> globalVec(maxLocalSize * m_WorldSize);
    MPI_Allgather(localVec.data(), maxLocalSize, MPI_CHAR, globalVec.data(),
                  maxLocalSize, MPI_CHAR, MPI_COMM_WORLD);

    // deserialize global metadata Json
    try
    {
        for (size_t i = 0; i < m_WorldSize; ++i)
        {
            if (globalVec[i * maxLocalSize] == '\0')
            {
                m_GlobalWritePatternJson[i] = nullptr;
            }
            else
            {
                m_GlobalWritePatternJson[i] = nlohmann::json::parse(
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
    ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern);

    for (const auto &blockVec : m_GlobalWritePattern)
    {
        for (const auto &b : blockVec)
        {
            if (b.type.empty())
            {
                throw(std::runtime_error("unknown data type"));
            }
#define declare_type(T)                                                        \
    else if (b.type == helper::GetType<T>())                                   \
    {                                                                          \
        auto v = m_IO.InquireVariable<T>(b.name);                              \
        if (not v)                                                             \
        {                                                                      \
            m_IO.DefineVariable<T>(b.name, b.shape, b.start, b.shape);         \
        }                                                                      \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else { throw(std::runtime_error("unknown data type")); }
        }
    }

    for (int i = 0; i < m_WorldSize; ++i)
    {
        if (m_GlobalWritePatternJson[i] == nullptr)
        {
            continue;
        }
        auto &attributesJson = m_GlobalWritePatternJson[i]["Attributes"];
        if (attributesJson == nullptr)
        {
            continue;
        }
        for (const auto &attributeJson : attributesJson)
        {
            const std::string type(attributeJson["Type"].get<std::string>());
            if (type.empty())
            {
            }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        const auto &attributesDataMap = m_IO.GetAttributesDataMap();           \
        auto it =                                                              \
            attributesDataMap.find(attributeJson["Name"].get<std::string>());  \
        if (it == attributesDataMap.end())                                     \
        {                                                                      \
            if (attributeJson["IsSingleValue"].get<bool>())                    \
            {                                                                  \
                m_IO.DefineAttribute<T>(                                       \
                    attributeJson["Name"].get<std::string>(),                  \
                    attributeJson["Value"].get<T>());                          \
            }                                                                  \
            else                                                               \
            {                                                                  \
                m_IO.DefineAttribute<T>(                                       \
                    attributeJson["Name"].get<std::string>(),                  \
                    attributeJson["Array"].get<std::vector<T>>().data(),       \
                    attributeJson["Array"].get<std::vector<T>>().size());      \
            }                                                                  \
        }                                                                      \
    }
            ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
        }
    }
}

void SscReader::SyncReadPattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncReadPattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    std::string localStr = m_LocalReadPatternJson.dump();

    // aggregate global read pattern
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  MPI_COMM_WORLD);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.c_str(), localStr.size());
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

    ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern);
    ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
    m_AllReceivingWriterRanks = ssc::AllOverlapRanks(m_GlobalWritePattern);
    CalculatePosition(m_GlobalWritePattern, m_AllReceivingWriterRanks);
    size_t totalDataSize = 0;
    for (auto i : m_AllReceivingWriterRanks)
    {
        totalDataSize += i.second.second;
    }
    m_Buffer.resize(totalDataSize);

    if (m_Verbosity >= 10)
    {
        ssc::PrintBlockVec(m_LocalReadPattern, "Local Read Pattern");
    }
}

void SscReader::CalculatePosition(ssc::BlockVecVec &bvv,
                                  ssc::RankPosMap &allRanks)
{
    TAU_SCOPED_TIMER_FUNC();

    size_t bufferPosition = 0;

    for (size_t rank = 0; rank < bvv.size(); ++rank)
    {
        bool hasOverlap = false;
        for (const auto r : allRanks)
        {
            if (r.first == rank)
            {
                hasOverlap = true;
                break;
            }
        }
        if (hasOverlap)
        {
            allRanks[rank].first = bufferPosition;
            auto &bv = bvv[rank];
            for (auto &b : bv)
            {
                b.bufferStart += bufferPosition;
            }
            size_t currentRankTotalSize = TotalDataSize(bv);
            allRanks[rank].second = currentRankTotalSize + 1;
            bufferPosition += currentRankTotalSize + 1;
        }
        else
        {
            bvv[rank].clear();
        }
    }
}

#define declare_type(T)                                                        \
    void SscReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void SscReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    SscReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        return AllStepsBlocksInfoCommon(variable);                             \
    }                                                                          \
    std::vector<typename Variable<T>::Info> SscReader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return BlocksInfoCommon(variable, step);                               \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SscReader::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::DoClose, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }
    MPI_Win_free(&m_MpiWin);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
