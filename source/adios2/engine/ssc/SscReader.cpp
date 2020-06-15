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

    helper::GetParameter(m_IO.m_Parameters, "MpiMode", m_MpiMode);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "MaxFilenameLength",
                         m_MaxFilenameLength);
    helper::GetParameter(m_IO.m_Parameters, "RendezvousAppCount",
                         m_RendezvousAppCount);
    helper::GetParameter(m_IO.m_Parameters, "MaxStreamsPerApp",
                         m_MaxStreamsPerApp);
    helper::GetParameter(m_IO.m_Parameters, "OpenTimeoutSecs",
                         m_OpenTimeoutSecs);

    m_Buffer.resize(1);

    SyncMpiPattern();
    m_ReaderRank = m_Comm.Rank();
    m_ReaderSize = m_Comm.Size();
    MPI_Comm_rank(m_StreamComm, &m_StreamRank);
    MPI_Comm_size(m_StreamComm, &m_StreamSize);

    m_GlobalWritePattern.resize(m_StreamSize);
}

SscReader::~SscReader() { TAU_SCOPED_TIMER_FUNC(); }

void SscReader::GetOneSidedPostPush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_post(m_MpiAllWritersGroup, 0, m_MpiWin);
}

void SscReader::GetOneSidedFencePush()
{
    TAU_SCOPED_TIMER_FUNC();
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
}

void SscReader::GetTwoSided()
{
    TAU_SCOPED_TIMER_FUNC();
    for (const auto &i : m_AllReceivingWriterRanks)
    {
        m_MpiRequests.emplace_back();
        MPI_Irecv(m_Buffer.data() + i.second.first, i.second.second, MPI_CHAR,
                  i.first, 0, m_StreamComm, &m_MpiRequests.back());
    }
}

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_InitialStep)
    {
        m_InitialStep = false;
        SyncWritePattern();
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, m_StreamComm, &m_MpiWin);
        MPI_Win_start(m_MpiAllWritersGroup, 0, m_MpiWin);
    }
    else
    {
        ++m_CurrentStep;
        if (m_MpiMode == "twosided")
        {
            MPI_Status statuses[m_MpiRequests.size()];
            MPI_Waitall(m_MpiRequests.size(), m_MpiRequests.data(), statuses);
            m_MpiRequests.clear();
        }
        else if (m_MpiMode == "onesidedfencepush")
        {
            MPI_Win_fence(0, m_MpiWin);
        }
        else if (m_MpiMode == "onesidedpostpush")
        {
            MPI_Win_wait(m_MpiWin);
        }
        else if (m_MpiMode == "onesidedfencepull")
        {
            MPI_Win_fence(0, m_MpiWin);
        }
        else if (m_MpiMode == "onesidedpostpull")
        {
            MPI_Win_complete(m_MpiWin);
        }
    }

    for (const auto &r : m_GlobalWritePattern)
    {
        for (auto &v : r)
        {
            if (v.shapeId == ShapeID::GlobalValue ||
                v.shapeId == ShapeID::LocalValue)
            {
                std::vector<char> value(v.bufferCount);
                if (m_CurrentStep == 0)
                {
                    std::memcpy(value.data(), v.value.data(), v.value.size());
                }
                else
                {
                    std::memcpy(value.data(), m_Buffer.data() + v.bufferStart,
                                v.bufferCount);
                }
                if (v.type == DataType::None)
                {
                    throw(std::runtime_error("unknown data type"));
                }
                else if (v.type == DataType::String)
                {
                    auto variable = m_IO.InquireVariable<std::string>(v.name);
                    if (variable)
                    {
                        variable->m_Value =
                            std::string(value.begin(), value.end());
                        variable->m_Min =
                            std::string(value.begin(), value.end());
                        variable->m_Max =
                            std::string(value.begin(), value.end());
                    }
                }
#define declare_type(T)                                                        \
    else if (v.type == helper::GetDataType<T>())                               \
    {                                                                          \
        auto variable = m_IO.InquireVariable<T>(v.name);                       \
        if (variable)                                                          \
        {                                                                      \
            std::memcpy(&variable->m_Min, value.data(), value.size());         \
            std::memcpy(&variable->m_Max, value.data(), value.size());         \
            std::memcpy(&variable->m_Value, value.data(), value.size());       \
        }                                                                      \
    }
                ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                else { throw(std::runtime_error("unknown data type")); }
            }
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::BeginStep, World Rank " << m_StreamRank
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
                       m_StreamComm, &m_MpiWin);
    }

    if (m_MpiMode == "twosided")
    {
        GetTwoSided();
    }
    else if (m_MpiMode == "onesidedfencepush")
    {
        GetOneSidedFencePush();
    }
    else if (m_MpiMode == "onesidedpostpush")
    {
        GetOneSidedPostPush();
    }
    else if (m_MpiMode == "onesidedfencepull")
    {
        GetOneSidedFencePull();
    }
    else if (m_MpiMode == "onesidedpostpull")
    {
        GetOneSidedPostPull();
    }
}

// PRIVATE

void SscReader::SyncMpiPattern()
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncMpiPattern, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    m_MpiHandshake.Handshake(m_Name, 'r', m_OpenTimeoutSecs, m_MaxStreamsPerApp,
                             m_MaxFilenameLength, m_RendezvousAppCount,
                             CommAsMPI(m_Comm));

    std::vector<int> allStreamRanks;
    std::vector<int> allWriterRanks;

    for (const auto &app : m_MpiHandshake.GetWriterMap(m_Name))
    {
        for (int rank : app.second)
        {
            allWriterRanks.push_back(rank);
            allStreamRanks.push_back(rank);
        }
    }

    for (const auto &app : m_MpiHandshake.GetReaderMap(m_Name))
    {
        for (int rank : app.second)
        {
            allStreamRanks.push_back(rank);
        }
    }

    MPI_Group worldGroup;
    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    MPI_Group_incl(worldGroup, allWriterRanks.size(), allWriterRanks.data(),
                   &m_MpiAllWritersGroup);

    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    std::sort(allStreamRanks.begin(), allStreamRanks.end());
    MPI_Group allWorkersGroup;
    MPI_Group_incl(worldGroup, allStreamRanks.size(), allStreamRanks.data(),
                   &allWorkersGroup);
    MPI_Comm_create_group(MPI_COMM_WORLD, allWorkersGroup, 0, &m_StreamComm);
}

void SscReader::SyncWritePattern()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncWritePattern, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    // aggregate global write pattern
    size_t localSize = 0;
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  m_StreamComm);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::vector<char> globalVec(maxLocalSize * m_StreamSize);
    MPI_Allgather(localVec.data(), maxLocalSize, MPI_CHAR, globalVec.data(),
                  maxLocalSize, MPI_CHAR, m_StreamComm);

    // deserialize global metadata Json
    ssc::LocalJsonToGlobalJson(globalVec, maxLocalSize, m_StreamSize,
                               m_GlobalWritePatternJson);

    // deserialize variables metadata
    ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern);

    for (const auto &blockVec : m_GlobalWritePattern)
    {
        for (const auto &b : blockVec)
        {
            if (b.type == DataType::None)
            {
                throw(std::runtime_error("unknown data type"));
            }
#define declare_type(T)                                                        \
    else if (b.type == helper::GetDataType<T>())                               \
    {                                                                          \
        auto v = m_IO.InquireVariable<T>(b.name);                              \
        if (not v)                                                             \
        {                                                                      \
            Dims vStart = b.start;                                             \
            Dims vShape = b.shape;                                             \
            if (!helper::IsRowMajor(m_IO.m_HostLanguage))                      \
            {                                                                  \
                std::reverse(vStart.begin(), vStart.end());                    \
                std::reverse(vShape.begin(), vShape.end());                    \
            }                                                                  \
            if (b.shapeId == ShapeID::GlobalValue)                             \
            {                                                                  \
                m_IO.DefineVariable<T>(b.name);                                \
            }                                                                  \
            else if (b.shapeId == ShapeID::GlobalArray)                        \
            {                                                                  \
                m_IO.DefineVariable<T>(b.name, vShape, vStart, vShape);        \
            }                                                                  \
            else if (b.shapeId == ShapeID::LocalValue)                         \
            {                                                                  \
                m_IO.DefineVariable<T>(b.name, {adios2::LocalValueDim});       \
            }                                                                  \
            else if (b.shapeId == ShapeID::LocalArray)                         \
            {                                                                  \
                m_IO.DefineVariable<T>(b.name, {}, {}, vShape);                \
            }                                                                  \
        }                                                                      \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else { throw(std::runtime_error("unknown data type")); }
        }
    }

    for (int i = 0; i < m_StreamSize; ++i)
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
            const DataType type(helper::GetDataTypeFromString(
                attributeJson["Type"].get<std::string>()));
            if (type == DataType::None)
            {
            }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
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
        std::cout << "SscReader::SyncReadPattern, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    std::string localStr = m_LocalReadPatternJson.dump();

    // aggregate global read pattern
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    MPI_Allreduce(&localSize, &maxLocalSize, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX,
                  m_StreamComm);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.c_str(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_StreamSize);
    MPI_Allgather(localVec.data(), maxLocalSize, MPI_CHAR, globalVec.data(),
                  maxLocalSize, MPI_CHAR, m_StreamComm);

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

    ssc::JsonToBlockVecVec(m_GlobalWritePatternJson, m_GlobalWritePattern);
    m_AllReceivingWriterRanks =
        ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
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
            size_t currentRankTotalSize = ssc::TotalDataSize(bv);
            allRanks[rank].second = currentRankTotalSize + 1;
            bufferPosition += currentRankTotalSize + 1;
        }
    }
}

#define declare_type(T)                                                        \
    void SscReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
        PerformGets();                                                         \
    }                                                                          \
    void SscReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
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
        std::cout << "SscReader::DoClose, World Rank " << m_StreamRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }
    MPI_Win_free(&m_MpiWin);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
