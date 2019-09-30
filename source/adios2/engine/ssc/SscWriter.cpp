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

    SyncMpiPattern();
}

StepStatus SscWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();

    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::BeginStep, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }

    if(m_InitialStep)
    {
        m_InitialStep=false;
        SyncWritePattern();
        SyncReadPattern();
    }
    else{
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
    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::EndStep, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }
}

void SscWriter::Flush(const int transportIndex) { TAU_SCOPED_TIMER_FUNC(); }

// PRIVATE

void SscWriter::SyncMpiPattern()
{
    int readerMasterWorldRank = 0;
    int writerMasterWorldRank = 0;
    if(m_WriterRank == 0)
    {
        writerMasterWorldRank = m_WorldRank;
    }
    MPI_Allreduce(&readerMasterWorldRank, &m_ReaderMasterWorldRank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&writerMasterWorldRank, &m_WriterMasterWorldRank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    if(m_WorldSize == m_WriterSize)
    {
        throw(std::runtime_error("no readers are found"));
    }

    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncMpiPattern, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }
}

void SscWriter::SyncWritePattern()
{
    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncWritePattern, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }

    // serialize local writer rank metadata
    nlohmann::json j;
    auto variables = m_IO.GetAvailableVariables();
    size_t varIndex = 0;
    for(auto &i : variables)
    {
        std::string type = i.second["Type"];
        Dims start;
        Dims count;
        Dims shape;

        if(type.empty())
        {
            throw(std::runtime_error("unknown data type"));
        }
#define declare_type(T)                                                        \
        else if (type == helper::GetType<T>())                                   \
        {                                                                          \
            auto var = m_IO.InquireVariable<T>(i.first);\
            shape = var->m_Shape;\
            start = var->m_Start;\
            count = var->m_Count;\
        }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        else {
            throw(std::runtime_error("unknown data type"));
        }

        auto &jref = j[i.first];
        jref["T"] = type;
        jref["S"] = shape;
        jref["O"] = start;
        jref["C"] = count;

        auto &mref = m_LocalWritePatternMap[i.first];
        mref.type = type;
        mref.shape = shape;
        mref.start = start;
        mref.count = count;
        mref.index = varIndex;

        ++varIndex;
    }
    std::string localStr = j.dump();

    // aggregate global metadata across all writers
    size_t localSize = localStr.size();
    size_t maxLocalSize;
    m_Comm.Allreduce(&localSize, &maxLocalSize, 1, MPI_MAX);
    std::vector<char> localVec(maxLocalSize, '\0');
    std::memcpy(localVec.data(), localStr.data(), localStr.size());
    std::vector<char> globalVec(maxLocalSize * m_WriterSize);
    m_Comm.GatherArrays(localVec.data(), maxLocalSize, globalVec.data(), 0);

    std::string globalStr;
    if(m_WriterRank ==0)
    {
        nlohmann::json globalJson;
        try{
            for(size_t i=0; i<m_WriterSize; ++i)
            {
                globalJson[i] = nlohmann::json::parse(globalVec.begin()+i*maxLocalSize, globalVec.begin()+(i+1)*maxLocalSize);
            }
        }
        catch(...)
        {
            throw(std::runtime_error("writer received corrupted aggregated metadata"));
        }
        // TODO: Add attributes
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
    std::memcpy(globalVec.data(), globalStr.data(), globalStr.size());

    // sync with readers
    MPI_Win win;
    MPI_Win_create(globalVec.data(), globalVec.size(), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

}

void SscWriter::SyncReadPattern()
{

    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncReadPattern, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }

    size_t globalSizeSrc = 0;
    size_t globalSizeDst = 0;
    MPI_Allreduce(&globalSizeSrc, &globalSizeDst, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);

    std::vector<char> globalVec(globalSizeDst);

    MPI_Win win;
    MPI_Win_create(globalVec.data(), globalVec.size(), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Get(globalVec.data(), globalVec.size(), MPI_CHAR, m_ReaderMasterWorldRank,0,globalVec.size(), MPI_CHAR, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    nlohmann::json j;
    try{
        j = nlohmann::json::parse(globalVec);
    }
    catch(...)
    {
        throw(std::runtime_error("writer received corrupted read pattern"));
    }

    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncReadPattern, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << " obtained reading requests: " << j.dump(4) << std::endl;
    }

    m_GlobalReadPatternMap.resize(m_ReaderSize);

    int rank = 0;
    for(auto &rankj : j)
    {
        int varIndex = 0;
        for(auto it = rankj.begin(); it!=rankj.end(); ++it)
        {
            auto &v = m_GlobalReadPatternMap[rank][it.key()];
            v.type = it.value()["T"].get<std::string>();
            v.start = it.value()["O"].get<Dims>();
            v.count = it.value()["C"].get<Dims>();
            v.index = varIndex;
            ++varIndex;
        }
        ++rank;
    }

}

#define declare_type(T)                                                        \
    void SscWriter::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
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
    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::DoClose, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
