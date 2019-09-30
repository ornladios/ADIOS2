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
#include "nlohmann/json.hpp"
#include "adios2/helper/adiosFunctions.h"

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

    SyncRank();
    SyncMetadata();
}

SscReader::~SscReader() { TAU_SCOPED_TIMER_FUNC(); }

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::BeginStep, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    if(m_InitialStep)
    {
        m_InitialStep = false;
        SyncRequests();
    }
    else{
        ++m_CurrentStep;
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
    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::EndStep, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }
}

// PRIVATE

void SscReader::SyncRank()
{
    int readerMasterWorldRank = 0;
    int writerMasterWorldRank = 0;
    if(m_ReaderRank == 0)
    {
        readerMasterWorldRank = m_WorldRank;
    }
    MPI_Allreduce(&readerMasterWorldRank, &m_ReaderMasterWorldRank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&writerMasterWorldRank, &m_WriterMasterWorldRank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    if(m_WorldSize == m_ReaderSize)
    {
        throw(std::runtime_error("no writers are found"));
    }

    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::SyncRank, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }
}

void SscReader::SyncMetadata()
{
    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::SyncMetadata, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    //sync
    size_t globalSizeDst = 0;
    size_t globalSizeSrc = 0;
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);
    std::vector<char> globalVec(globalSizeDst);

    MPI_Win win;
    MPI_Win_create(NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Get(globalVec.data(), globalVec.size(), MPI_CHAR, 0,0,globalVec.size(), MPI_CHAR, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    //deserialize
    nlohmann::json j;
    try{
        j = nlohmann::json::parse(globalVec);
    }
    catch(...)
    {
        throw(std::runtime_error("reader received corrupted metadata"));
    }

    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::SyncMetadata obtained metadata: "<< j.dump(4) << std::endl;
    }

    m_GlobalWriterVarInfoMap.resize(m_WriterSize);
    int rank = 0;
    for(auto &rankj : j)
    {
        for(auto itVar = rankj.begin(); itVar != rankj.end(); ++itVar)
        {
            std::string type = itVar.value()["T"].get<std::string>();
            Dims shape = itVar.value()["S"].get<Dims>();
            Dims start = itVar.value()["O"].get<Dims>();
            Dims count = itVar.value()["C"].get<Dims>();

            if(rank == 0)
            {
                if(type.empty())
                {
                    throw(std::runtime_error("unknown data type"));
                }
#define declare_type(T)                                                        \
                else if (type == helper::GetType<T>())                                   \
                {                                                                          \
                    m_IO.DefineVariable<T>(itVar.key(), shape, start, shape);\
                    m_LocalReaderVarInfoMap[itVar.key()].type = type;\
                }
                ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                else {
                    throw(std::runtime_error("unknown data type"));
                }
            }
            else if(rank == m_ReaderSize)
            {
                // TODO: Parse attributes
            }
            else
            {
                auto &mapRef = m_GlobalWriterVarInfoMap[rank][itVar.key()];
                mapRef.shape = shape;
                mapRef.start = start;
                mapRef.count = count;
                mapRef.type = type;
            }
        }
        ++rank;
    }

}

void SscReader::SyncRequests()
{
    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::SyncRequests, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    //serialize
    for(auto &var : m_LocalReaderVarInfoMap)
    {
        if(var.second.type.empty())
        {
            throw(std::runtime_error("unknown data type"));
        }
#define declare_type(T)                                                        \
        else if (var.second.type == helper::GetType<T>())                                   \
        {                                                                          \
            auto v = m_IO.InquireVariable<T>(var.first);\
            if(v) {\
                var.second.count = v->m_Count;\
                var.second.start = v->m_Start;\
            }\
        }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        else {
            throw(std::runtime_error("unknown data type"));
        }
    }

    nlohmann::json j;
    for(auto &var : m_LocalReaderVarInfoMap)
    {
        j[var.first]["T"] = var.second.type;
        j[var.first]["O"] = var.second.start;
        j[var.first]["C"] = var.second.count;
    }

    std::string localStr = j.dump();

    // aggregate global read pattern across all readers
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    m_Comm.Allreduce(&localSize, &maxLocalSize, 1, MPI_MAX);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.c_str(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_ReaderSize);
    m_Comm.GatherArrays(localVec.data(), maxLocalSize, globalVec.data(), 0);

    std::string globalStr;
    if(m_ReaderRank == 0)
    {
        nlohmann::json globalJson;
        try{
            for(size_t i=0; i<m_ReaderSize; ++i)
            {
                globalJson[i] = nlohmann::json::parse(globalVec.begin()+i*maxLocalSize, globalVec.begin()+(i+1)*maxLocalSize);
            }
        }
        catch(...)
        {
            throw(std::runtime_error("reader received corrupted aggregated read pattern"));
        }
        globalStr = globalJson.dump();
    }


    size_t globalSizeSrc = globalStr.size();
    size_t globalSizeDst = 0;
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);

    if(globalStr.size() < globalSizeDst)
    {
        globalStr.resize(globalSizeDst);
    }

    globalVec.resize(globalSizeDst);
    std::memcpy(globalVec.data(), globalStr.data(), globalStr.size() );

    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::SyncRequests, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << ", serialized global read pattern: " << globalStr << std::endl;
    }

    // sync with writers
    MPI_Win win;
    MPI_Win_create(globalVec.data(), globalVec.size(), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);
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
    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::DoClose, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
