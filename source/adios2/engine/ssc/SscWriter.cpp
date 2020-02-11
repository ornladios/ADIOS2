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

#include "adios2/helper/adiosCommMPI.h"

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

    auto it = m_IO.m_Parameters.find("MpiMode");
    if(it != m_IO.m_Parameters.end())
    {
        m_MpiMode = it->second;
    }

    m_GlobalWritePattern.resize(m_WriterSize);
    m_GlobalReadPattern.resize(m_ReaderSize);
    m_Buffer.resize(1);
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


void SscWriter::PutOneSidedPostPush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_start(m_MpiAllReadersGroup,0,m_MpiWin);
    for (const auto &i : m_AllSendingReaderRanks)
    {
        MPI_Put(m_Buffer.data(), m_Buffer.size(), MPI_CHAR, m_ReaderMasterWorldRank + i.first, i.second.first, m_Buffer.size(), MPI_CHAR, m_MpiWin);
    }
    MPI_Win_complete(m_MpiWin);
}

void SscWriter::PutOneSidedFencePush()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Win_fence(0, m_MpiWin);
    for (const auto &i : m_AllSendingReaderRanks)
    {
        MPI_Put(m_Buffer.data(), m_Buffer.size(), MPI_CHAR, m_ReaderMasterWorldRank + i.first, i.second.first, m_Buffer.size(), MPI_CHAR, m_MpiWin);
    }
    MPI_Win_fence(0, m_MpiWin);
}

void SscWriter::PutTwoSided()
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<MPI_Request> requests;
    for (const auto &i : m_AllSendingReaderRanks)
    {
        requests.emplace_back();
        MPI_Isend(m_Buffer.data(), m_Buffer.size(), MPI_CHAR, m_ReaderMasterWorldRank + i.first, 0, MPI_COMM_WORLD, &requests.back());
    }
    for(auto &r : requests)
    {
        MPI_Status s;
        MPI_Wait(&r,&s);
    }
}

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
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &m_MpiWin);
    }

    if(m_MpiMode == "OneSidedFencePush")
    {
        PutOneSidedFencePush();
    }
    else if(m_MpiMode == "OneSidedPostPush")
    {
        PutOneSidedPostPush();
    }
    else if(m_MpiMode == "TwoSided")
    {
        PutTwoSided();
    }

}

void SscWriter::Flush(const int transportIndex) { TAU_SCOPED_TIMER_FUNC(); }

// PRIVATE

void SscWriter::SyncMpiPattern()
{
    TAU_SCOPED_TIMER_FUNC();
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

    std::vector<int> lrbuf;
    std::vector<int> grbuf;

    // Process m_WorldRank == 0 to gather all the local rank m_WriterRank, and
    // find out all the m_WriterRank == 0
    if (m_WorldRank == 0)
    {
        grbuf.resize(m_WorldSize);
    }

    MPI_Gather(&m_WriterRank, 1, MPI_INT, grbuf.data(), 1, MPI_INT, 0,
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
    if (m_WriterRank == 0) // Send type from each local rank 0 process to the
                           // world rank 0 process
    {
        if (m_WorldRank == 0) // App_ID
        {
            AppType.resize(m_AppSize);
            for (int i = 0; i < m_AppSize; ++i)
            {
                if (i == 0)
                {
                    AppType[i] = 0;
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
            int tmp = 0; // type 0 for writer
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
    if (m_WriterRank == 0) // Send m_AppID to each local rank 0 process
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

    m_Comm.Bcast(&m_AppID, 1, 0); // Local rank 0 process broadcast the m_AppID
                                  // within the local communicator.

    MPI_Bcast(&m_AppSize, 1, MPI_INT, 0, MPI_COMM_WORLD); // Bcast the m_AppSize

    // In each local communicator, each local rank 0 process gathers the world
    // rank of all the rest local processes.
    if (m_WriterRank == 0)
    {
        lrbuf.resize(m_WriterSize);
    }
    m_Comm.Gather(&m_WorldRank, 1, lrbuf.data(), 1, 0);

    // Send the WorldRank vector of each local communicator to the m_WorldRank
    // == 0 process.
    int WriterInfoSize = 0;
    int ReaderInfoSize = 0;
    if (m_WriterRank == 0)
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
            MPI_Send(&m_WriterSize, 1, MPI_INT, 0, 96, MPI_COMM_WORLD);
            MPI_Send(lrbuf.data(), lrbuf.size(), MPI_INT, 0, 98,
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
    MPI_Group_incl(worldGroup, m_AllReaderRanks.size(), m_AllReaderRanks.data(), &m_MpiAllReadersGroup);

    if (m_Verbosity >= 10)
    {
        std::cout << "WorldRank " << m_WorldRank << std::endl;
        std::cout << "AppID " << m_AppID << std::endl;
        std::cout << "AppSize " << m_AppSize << std::endl;
        std::cout << "m_WriterGlobalMpiInfo have:" << std::endl;
        for (int i = 0; i < m_WriterGlobalMpiInfo.size(); ++i)
        {
            std::cout << "Vector " << i << ": ";
            for (int j = 0; j < m_WriterGlobalMpiInfo[i].size(); ++j)
            {
                std::cout << m_WriterGlobalMpiInfo[i][j] << "  ";
            }
            std::cout << std::endl;
        }

        std::cout << "m_ReaderGlobalMpiInfo have:" << std::endl;
        for (int i = 0; i < m_ReaderGlobalMpiInfo.size(); ++i)
        {
            std::cout << "Vector " << i << ": ";
            for (int j = 0; j < m_ReaderGlobalMpiInfo[i].size(); ++j)
            {
                std::cout << m_ReaderGlobalMpiInfo[i][j] << "  ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        std::cout << std::endl;
    }
}

void SscWriter::SyncWritePattern()
{
    TAU_SCOPED_TIMER_FUNC();
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
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);

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

}

void SscWriter::SyncReadPattern()
{
    TAU_SCOPED_TIMER_FUNC();
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
    MPI_Get(globalVec.data(), globalVec.size(), MPI_CHAR, m_ReaderMasterWorldRank, 0, globalVec.size(), MPI_CHAR, win);
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
                currentReaderOverlapWriterRanks[rank].second = currentRankTotalSize;
                bufferPosition += currentRankTotalSize;
            }
        }
        allOverlapRanks[overlapRank.first] = currentReaderOverlapWriterRanks[writerRank];
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

    if(m_MpiMode == "OneSidedFencePush")
    {
        MPI_Win_fence(0, m_MpiWin);
        if (m_WriterRank == 0)
        {
            for (int i = 0; i < m_ReaderSize; ++i)
            {
                MPI_Put(m_Buffer.data(), 1, MPI_CHAR, m_ReaderMasterWorldRank + i, 0, 1, MPI_CHAR, m_MpiWin);
            }
        }
        MPI_Win_fence(0, m_MpiWin);
    }
    else if(m_MpiMode == "OneSidedPostPush")
    {
        MPI_Win_start(m_MpiAllReadersGroup,0,m_MpiWin);
        for (const auto &i : m_AllSendingReaderRanks)
        {
            MPI_Put(m_Buffer.data(), 1, MPI_CHAR, m_ReaderMasterWorldRank + i.first, 0, 1, MPI_CHAR, m_MpiWin);
        }
        MPI_Win_complete(m_MpiWin);
    }
    else if(m_MpiMode == "TwoSided")
    {
        std::vector<MPI_Request> requests;
        for (const auto &i : m_AllSendingReaderRanks)
        {
            requests.emplace_back();
            MPI_Isend(m_Buffer.data(), 1, MPI_CHAR, m_ReaderMasterWorldRank + i.first, 0, MPI_COMM_WORLD, &requests.back());
        }
        for(auto &r : requests)
        {
            MPI_Status s;
            MPI_Wait(&r,&s);
        }
    }

    MPI_Win_free(&m_MpiWin);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
