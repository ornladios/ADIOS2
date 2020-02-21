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

    auto it = m_IO.m_Parameters.find("MpiMode");
    if (it != m_IO.m_Parameters.end())
    {
        m_MpiMode = it->second;
    }

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
    for (auto &r : requests)
    {
        MPI_Status s;
        MPI_Wait(&r, &s);
    }
}

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::BeginStep, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    if (m_InitialStep)
    {
        m_InitialStep = false;
        SyncReadPattern();
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), 1, MPI_INFO_NULL,
                       MPI_COMM_WORLD, &m_MpiWin);
    }
    else
    {
        ++m_CurrentStep;
    }

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
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::EndStep, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }
}

// PRIVATE

void SscReader::SyncMpiPattern()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncMpiPattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    TAU_SCOPED_TIMER_FUNC();
    if (m_WorldSize == m_ReaderSize)
    {
        throw(std::runtime_error("no writers are found"));
    }

    std::vector<int> lrbuf;
    std::vector<int> grbuf;

    // Process m_WorldRank == 0 to gather all the local rank m_WriterRank, and
    // find out all the m_WriterRank == 0
    if (m_WorldRank == 0)
    {
        grbuf.resize(m_WorldSize);
    }

    MPI_Gather(&m_ReaderRank, 1, MPI_INT, grbuf.data(), 1, MPI_INT, 0,
               MPI_COMM_WORLD);

    std::vector<int> AppStart; // m_WorldRank of the local rank 0 process
    if (m_WorldRank == 0)
    {
        for (int i = 0; i < m_WorldSize; ++i)
        {
            if (grbuf[i] == 0)
            {
                AppStart.push_back(i);
            }
        }
        m_AppSize = AppStart.size();
    }

    // Each local rank 0 process send their type (0 for writer, 1 for reader) to
    // the world rank 0 process The AppStart are re-ordered to put all writers
    // ahead of all the readers.
    std::vector<int>
        AppType; // Vector to record the type of the local rank 0 process
    if (m_ReaderRank == 0) // Send type from each local rank 0 process to the
                           // world rank 0 process
    {
        if (m_WorldRank == 0) // App_ID
        {
            AppType.resize(m_AppSize);
            for (int i = 0; i < m_AppSize; ++i)
            {
                if (i == 0)
                {
                    AppType[i] = 1;
                    ;
                }
                else
                {
                    int tmp = 1;
                    MPI_Recv(&tmp, 1, MPI_INT, AppStart[i], 96, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
                    AppType[i] = tmp;
                }
            }
        }
        else
        {
            int tmp = 1; // type 1 for reader
            MPI_Send(&tmp, 1, MPI_INT, 0, 96, MPI_COMM_WORLD); //
        }
    }

    if (m_WorldRank == 0)
    {
        std::vector<int> AppWriter;
        std::vector<int> AppReader;

        for (int i = 0; i < m_AppSize; ++i)
        {
            if (AppType[i] == 0)
            {
                AppWriter.push_back(AppStart[i]);
            }
            else
            {
                AppReader.push_back(AppStart[i]);
            }
        }
        m_WriterGlobalMpiInfo.resize(AppWriter.size());
        m_ReaderGlobalMpiInfo.resize(AppReader.size());
        AppStart = AppWriter;
        AppStart.insert(AppStart.end(), AppReader.begin(), AppReader.end());
    }

    // Send the m_AppSize and m_AppID to each local rank 0 process
    if (m_ReaderRank == 0) // Send m_AppID to each local rank 0 process
    {
        if (m_WorldRank == 0) // App_ID
        {
            for (int i = 0; i < m_AppSize; ++i)
            {
                MPI_Send(&i, 1, MPI_INT, AppStart[i], 99, MPI_COMM_WORLD); //
            }
        }
        else
        {
            MPI_Recv(&m_AppID, 1, MPI_INT, 0, 99, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }
    }

    m_Comm.Bcast(&m_AppID, sizeof(int),
                 0); // Local rank 0 process broadcast the m_AppID within the
                     // local communicator.

    MPI_Bcast(&m_AppSize, 1, MPI_INT, 0, MPI_COMM_WORLD); // Bcast the m_AppSize

    // In each local communicator, each local rank 0 process gathers the world
    // rank of all the rest local processes.
    if (m_ReaderRank == 0)
    {
        lrbuf.resize(m_ReaderSize);
    }

    m_Comm.Gather(&m_WorldRank, 1, lrbuf.data(), 1, 0);

    // Send the WorldRank vector of each local communicator to the m_WorldRank
    // == 0 process.
    int WriterInfoSize = 0;
    int ReaderInfoSize = 0;
    if (m_ReaderRank == 0)
    {
        if (m_WorldRank == 0) // App_ID
        {
            for (int i = 0; i < m_WriterGlobalMpiInfo.size(); ++i)
            {
                if (i == 0)
                {
                    m_WriterGlobalMpiInfo[i] = lrbuf;
                    ++WriterInfoSize;
                }
                else
                {
                    int j_writersize;
                    MPI_Recv(&j_writersize, 1, MPI_INT, AppStart[i], 96,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE); //
                    ++WriterInfoSize;

                    m_WriterGlobalMpiInfo[i].resize(j_writersize);
                    MPI_Recv(m_WriterGlobalMpiInfo[i].data(), j_writersize,
                             MPI_INT, AppStart[i], 98, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE); //
                }
            }

            for (int i = m_WriterGlobalMpiInfo.size(); i < m_AppSize; ++i)
            {
                if (i == 0)
                {
                    m_ReaderGlobalMpiInfo[i] = lrbuf;
                    ++ReaderInfoSize;
                }
                else
                {
                    int j_readersize;
                    MPI_Recv(&j_readersize, 1, MPI_INT, AppStart[i], 95,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE); //
                    ++ReaderInfoSize;

                    m_ReaderGlobalMpiInfo[i - m_WriterGlobalMpiInfo.size()]
                        .resize(j_readersize);
                    MPI_Recv(
                        m_ReaderGlobalMpiInfo[i - m_WriterGlobalMpiInfo.size()]
                            .data(),
                        j_readersize, MPI_INT, AppStart[i], 97, MPI_COMM_WORLD,
                        MPI_STATUS_IGNORE); //
                }
            }
        }
        else
        {
            MPI_Send(&m_ReaderSize, 1, MPI_INT, 0, 95, MPI_COMM_WORLD);
            MPI_Send(lrbuf.data(), lrbuf.size(), MPI_INT, 0, 97,
                     MPI_COMM_WORLD);
        }
    }

    // Broadcast m_WriterGlobalMpiInfo and m_ReaderGlobalMpiInfo to all the
    // processes.
    MPI_Bcast(&WriterInfoSize, 1, MPI_INT, 0,
              MPI_COMM_WORLD); // Broadcast writerinfo size
    MPI_Bcast(&ReaderInfoSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    m_WriterGlobalMpiInfo.resize(WriterInfoSize);
    m_ReaderGlobalMpiInfo.resize(ReaderInfoSize);

    for (int i = 0; i < WriterInfoSize; ++i)
    {
        int ilen;
        if (m_WorldRank == 0)
        {
            ilen = m_WriterGlobalMpiInfo[i].size();
        }
        MPI_Bcast(&ilen, 1, MPI_INT, 0, MPI_COMM_WORLD);
        m_WriterGlobalMpiInfo[i].resize(ilen);
        MPI_Bcast(m_WriterGlobalMpiInfo[i].data(), ilen, MPI_INT, 0,
                  MPI_COMM_WORLD); // Broadcast readerinfo size
    }

    for (int i = 0; i < ReaderInfoSize; ++i)
    {
        int ilen;
        if (m_WorldRank == 0)
        {
            ilen = m_ReaderGlobalMpiInfo[i].size();
        }
        MPI_Bcast(&ilen, 1, MPI_INT, 0, MPI_COMM_WORLD);
        m_ReaderGlobalMpiInfo[i].resize(ilen);
        MPI_Bcast(m_ReaderGlobalMpiInfo[i].data(), ilen, MPI_INT, 0,
                  MPI_COMM_WORLD); // Broadcast readerinfo size
    }

    for (const auto &app : m_WriterGlobalMpiInfo)
    {
        for (int rank : app)
        {
            m_AllWriterRanks.push_back(rank);
        }
    }

    for (const auto &app : m_ReaderGlobalMpiInfo)
    {
        for (int rank : app)
        {
            m_AllReaderRanks.push_back(rank);
        }
    }

    MPI_Group worldGroup;
    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    MPI_Group_incl(worldGroup, m_AllWriterRanks.size(), m_AllWriterRanks.data(),
                   &m_MpiAllWritersGroup);

    if (m_Verbosity >= 10 and m_WorldRank == 0)
    {
        ssc::PrintMpiInfo(m_WriterGlobalMpiInfo, m_ReaderGlobalMpiInfo);
    }
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
        if (globalJson[i] == nullptr)
        {
            continue;
        }
        auto &attributesJson = globalJson[i]["Attributes"];
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

    nlohmann::json readPatternJson;

    // serialize
    auto variables = m_IO.GetAvailableVariables();
    for (auto &var : variables)
    {
        m_LocalReadPattern.emplace_back();
        auto &b = m_LocalReadPattern.back();
        std::string type = var.second["Type"];
        if (type.empty())
        {
            throw(std::runtime_error("unknown data type"));
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        auto v = m_IO.InquireVariable<T>(var.first);                           \
        b.name = var.first;                                                    \
        b.count = v->m_Count;                                                  \
        b.start = v->m_Start;                                                  \
        b.shape = v->m_Shape;                                                  \
        b.type = type;                                                         \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        else { throw(std::runtime_error("unknown data type")); }

        for (const auto &d : b.count)
        {
            if (d == 0)
            {
                throw(std::runtime_error(
                    "SetSelection count dimensions cannot be 0"));
            }
        }

        readPatternJson["Variables"].emplace_back();
        auto &jref = readPatternJson["Variables"].back();
        jref["Name"] = var.first;
        jref["Type"] = type;
        jref["Start"] = b.start;
        jref["Count"] = b.count;
        jref["Shape"] = b.shape;
        jref["BufferStart"] = 0;
        jref["BufferCount"] = 0;
    }

    std::string localStr = readPatternJson.dump();

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
