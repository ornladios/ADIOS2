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
    m_WriterSize = m_WorldSize - m_ReaderSize;

    m_GlobalWritePattern.resize(m_WriterSize);

    SyncMpiPattern();
    SyncWritePattern();
}

SscReader::~SscReader() { TAU_SCOPED_TIMER_FUNC(); }

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
        MPI_Win_create(m_Buffer.data(), m_Buffer.size(), sizeof(char),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &m_MpiWin);
    }
    else
    {
        ++m_CurrentStep;
    }

    MPI_Win_fence(0, m_MpiWin);
    MPI_Win_fence(0, m_MpiWin);

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
    int readerMasterWorldRank = 0;
    int writerMasterWorldRank = 0;
    if (m_ReaderRank == 0)
    {
        readerMasterWorldRank = m_WorldRank;
    }
    MPI_Allreduce(&readerMasterWorldRank, &m_ReaderMasterWorldRank, 1, MPI_INT,
                  MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&writerMasterWorldRank, &m_WriterMasterWorldRank, 1, MPI_INT,
                  MPI_MAX, MPI_COMM_WORLD);

    if (m_WorldSize == m_ReaderSize)
    {
        throw(std::runtime_error("no writers are found"));
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncMpiPattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }


    std::vector<int> lrbuf;
    std::vector<int> grbuf;
    std::vector<int> m_tmp;
    int m_AppID = 0;
    int m_AppSize = 0;  //Total number of applications.
    std::vector<std::vector<int>> m_WriterGlobalMpiInfo;
    std::vector<std::vector<int>> m_ReaderGlobalMpiInfo;


    // Process m_WorldRank == 0 to gather all the local rank m_WriterRank, and find out all the m_WriterRank == 0
    if (m_WorldRank == 0)
    {
        grbuf.resize(m_WorldSize);
    }

    m_tmp.push_back(m_ReaderRank);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather( m_tmp.data(), 1, MPI_INT, grbuf.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
    m_tmp.clear();
    MPI_Barrier(MPI_COMM_WORLD);
    /*if (m_WorldRank == 0)
    {
        //std::cout << "All local ranks:" << std::endl;
        for (int i = 0; i < grbuf.size(); ++i)
        {
            std::cout << grbuf[i] << ", ";
        }
        std::cout << std::endl << std::endl;
    }*/


    std::vector<int> AppStart;  // m_WorldRank of the local rank 0 process
    if (m_WorldRank == 0)
    {
        for ( int i = 0; i < m_WorldSize; ++i )
        {
            if (grbuf[i] == 0)
            {
                AppStart.push_back(i);
            }
        }
        m_AppSize = AppStart.size();
        //std::cout << "There are " << m_AppSize << " applications." << std::endl;
    }


    //Each local rank 0 process send their type (0 for writer, 1 for reader) to the world rank 0 process
    //The AppStart are re-ordered to put all writers ahead of all the readers.
    std::vector<int> AppType;  // Vector to record the type of the local rank 0 process
    if (m_ReaderRank == 0) // Send type from each local rank 0 process to the world rank 0 process
    {
        if (m_WorldRank == 0) // App_ID
        {
            //std::cout << "ReaderReceiving!" << std::endl;
            for (int i = 0; i < m_AppSize; ++i)
            {
                if (i == 0)
                {
                    AppType.push_back(1);
                }
                else
                {
                    m_tmp.resize(1);
                    MPI_Recv(m_tmp.data(), 1, MPI_INT, AppStart[i], 96, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    AppType.push_back(m_tmp[0]);
                    m_tmp.clear();
                }
            }
        }
        else
        {
            //std::cout << "Reader Sending!" << std::endl;
            m_tmp.push_back(1); // type 1 for reader
            MPI_Send( m_tmp.data(), 1, MPI_INT, 0, 96, MPI_COMM_WORLD); //
            m_tmp.clear();
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

        /*std::cout << "AppStart_Reader = ";
        for (int i = 0;  i < m_AppSize; ++i)
        {
            std::cout << AppStart[i] << "   ";
        }
        std::cout << std::endl;*/
    }


    // Send the m_AppSize and m_AppID to each local rank 0 process
    if (m_ReaderRank == 0) // Send m_AppID to each local rank 0 process
    {
        if (m_WorldRank == 0) // App_ID
        {
            for (int i = 0; i < m_AppSize; ++i)
            {
                m_tmp.push_back(i);
                MPI_Send( m_tmp.data(), 1, MPI_INT, AppStart[i], 99, MPI_COMM_WORLD); //
                m_tmp.clear();
            }
        }
        else
        {
            m_tmp.push_back(m_AppID);
            MPI_Recv(m_tmp.data(), 1, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            m_AppID = m_tmp[0];
            m_tmp.clear();
        }
    }

    m_tmp.push_back(m_AppID);
    m_Comm.Barrier();
    m_Comm.Bcast(m_tmp.data() , sizeof(int), 0);  // Local rank 0 process broadcast the m_AppID within the local communicator.
    m_AppID = m_tmp[0];
    m_tmp.clear();
    //std::cout << "World Process " << m_WorldRank << " is local Reader " << m_ReaderRank << ". AppID is " << m_AppID << ". " << std::endl;
    m_Comm.Barrier();


    m_tmp.push_back(m_AppSize);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast( m_tmp.data(), 1, MPI_INT, 0, MPI_COMM_WORLD); // Bcast the m_AppSize
    m_AppSize = m_tmp[0];
    m_tmp.clear();
    //std::cout << "World Process " << m_WorldRank << " is local Reader " << m_ReaderRank << ". AppSize is " << m_AppSize << ". " << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);


    // In each local communicator, each local rank 0 process gathers the world rank of all the rest local processes.
    if (m_ReaderRank == 0)
    {
        //std::cout << "Reader size is " << m_ReaderSize << "." << std::endl;
        lrbuf.resize(m_ReaderSize);
    }

    m_tmp.push_back(m_WorldRank);
    //std::cout << "Reader Local Rank to send: " << m_tmp[0] << std::endl;
    m_Comm.Gather(m_tmp.data(), 1, lrbuf.data(), 1, 0);
    m_tmp.clear();

    if (m_ReaderRank == 0)
    {
        std::cout << "App " << m_AppID << " has ";
        for (int i = 0; i < lrbuf.size(); ++i)
        {
            std::cout << "world rank " << lrbuf[i] << ", ";
        }
        std::cout << " in it." << std::endl;
    }


    // Send the WorldRank vector of each local communicator to the m_WorldRank == 0 process.
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
                    WriterInfoSize += lrbuf.size();
                }
                else
                {
                    m_tmp.resize(1);
                    MPI_Recv( m_tmp.data(), 1, MPI_INT, AppStart[i], 96, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //
                    int j_writersize = m_tmp[0];
                    WriterInfoSize += j_writersize;
                    m_tmp.clear();

                    m_tmp.resize(j_writersize);
                    MPI_Recv( m_tmp.data(), j_writersize, MPI_INT, AppStart[i], 98, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //
                    m_WriterGlobalMpiInfo[i] = m_tmp;
                    m_tmp.clear();
                }
            }

            for (int i = m_WriterGlobalMpiInfo.size(); i < m_AppSize; ++i)
            {
                if (i == 0)
                {
                    m_ReaderGlobalMpiInfo[i] = lrbuf;
                    ReaderInfoSize += lrbuf.size();
                }
                else
                {
                    m_tmp.resize(1);
                    MPI_Recv( m_tmp.data(), 1, MPI_INT, AppStart[i], 95, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //
                    int j_readersize = m_tmp[0];
                    ReaderInfoSize += j_readersize;
                    m_tmp.clear();

                    m_tmp.resize(j_readersize);
                    MPI_Recv( m_tmp.data(), 1, MPI_INT, AppStart[i], 97, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //
                    m_ReaderGlobalMpiInfo[i] = m_tmp;
                    m_tmp.clear();
                }
            }

        }
        else
        {
            m_tmp.push_back(m_ReaderSize);
            MPI_Send(m_tmp.data(), 1, MPI_INT, 0, 95, MPI_COMM_WORLD);
            m_tmp.clear();

            MPI_Send(lrbuf.data(), lrbuf.size(), MPI_INT, 0, 97, MPI_COMM_WORLD);
        }
    }

    std::cout << "Reader Reached???" << std::endl;

}

void SscReader::SyncWritePattern()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncWritePattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    // sync with writers
    size_t globalSizeDst = 0;
    size_t globalSizeSrc = 0;
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG,
                  MPI_MAX, MPI_COMM_WORLD);
    std::vector<char> globalVec(globalSizeDst);

    MPI_Win win;
    MPI_Win_create(NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Get(globalVec.data(), globalVec.size(), MPI_CHAR, 0, 0,
            globalVec.size(), MPI_CHAR, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    // deserialize

    m_GlobalWritePattern.resize(m_WriterSize);
    ssc::JsonToBlockVecVec(globalVec, m_GlobalWritePattern);

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

    nlohmann::json j;
    try
    {
        j = nlohmann::json::parse(globalVec);
    }
    catch (...)
    {
        throw(std::runtime_error("corrupted json string"));
    }

    for (const auto &attributeJson : j[m_WriterSize])
    {
        const std::string type(attributeJson["T"].get<std::string>());
        if (type.empty())
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        const auto &attributesDataMap = m_IO.GetAttributesDataMap();           \
        auto it =                                                              \
            attributesDataMap.find(attributeJson["N"].get<std::string>());     \
        if (it == attributesDataMap.end())                                     \
        {                                                                      \
            if (attributeJson["S"].get<bool>())                                \
            {                                                                  \
                m_IO.DefineAttribute<T>(attributeJson["N"].get<std::string>(), \
                                        attributeJson["V"].get<T>());          \
            }                                                                  \
            else                                                               \
            {                                                                  \
                m_IO.DefineAttribute<T>(                                       \
                    attributeJson["N"].get<std::string>(),                     \
                    attributeJson["V"].get<std::vector<T>>().data(),           \
                    attributeJson["V"].get<std::vector<T>>().size());          \
            }                                                                  \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void SscReader::SyncReadPattern()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncReadPattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    nlohmann::json j;

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

        j.emplace_back();
        auto &jref = j.back();
        jref["N"] = var.first;
        jref["T"] = type;
        jref["O"] = b.start;
        jref["C"] = b.count;
        jref["S"] = b.shape;
        jref["X"] = 0;
        jref["Y"] = 0;
    }

    std::string localStr = j.dump();

    // aggregate global read pattern across all readers
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    m_Comm.Allreduce(&localSize, &maxLocalSize, 1, helper::Comm::Op::Max);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.c_str(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_ReaderSize);
    m_Comm.GatherArrays(localVec.data(), maxLocalSize, globalVec.data(), 0);

    std::string globalStr;
    if (m_ReaderRank == 0)
    {
        nlohmann::json globalJson;
        try
        {
            for (size_t i = 0; i < m_ReaderSize; ++i)
            {
                globalJson[i] = nlohmann::json::parse(
                    globalVec.begin() + i * maxLocalSize,
                    globalVec.begin() + (i + 1) * maxLocalSize);
            }
        }
        catch (...)
        {
            throw(std::runtime_error(
                "reader received corrupted aggregated read pattern"));
        }
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

    if (m_Verbosity >= 5)
    {
        std::cout << "SscReader::SyncReadPattern, World Rank " << m_WorldRank
                  << ", Reader Rank " << m_ReaderRank
                  << ", serialized global read pattern: " << globalStr
                  << std::endl;
    }

    // sync with writers
    MPI_Win win;
    MPI_Win_create(globalVec.data(), globalVec.size(), sizeof(char),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    ssc::CalculateOverlap(m_GlobalWritePattern, m_LocalReadPattern);
    m_AllReceivingWriterRanks = ssc::AllOverlapRanks(m_GlobalWritePattern);
    CalculatePosition(m_GlobalWritePattern, m_AllReceivingWriterRanks);

    size_t totalDataSize = 0;
    for (auto i : m_AllReceivingWriterRanks)
    {
        totalDataSize += i.second.second;
    }

    m_Buffer.resize(totalDataSize + 1);

    if (m_Verbosity >= 10)
    {
        ssc::PrintBlockVec(m_LocalReadPattern, "Local Read Pattern");
    }
}

void SscReader::CalculatePosition(ssc::BlockVecVec &bvv,
                                  ssc::RankPosMap &allRanks)
{
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
            allRanks[rank].second = currentRankTotalSize;
            bufferPosition += currentRankTotalSize;
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
