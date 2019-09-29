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

    SyncRank();
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
        SerializeMetadata();
        SyncMetadata();
        SyncRequests();
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

void SscWriter::SyncRank()
{
    int readerMasterWorldRank = 0;
    int writerMasterWorldRank = 0;
    if(m_WriterRank == 0)
    {
        writerMasterWorldRank = m_WorldRank;
    }
    MPI_Allreduce(&readerMasterWorldRank, &m_ReaderMasterWorldRank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&writerMasterWorldRank, &m_WriterMasterWorldRank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
}

void SscWriter::SerializeMetadata()
{
    nlohmann::json j;
    auto variables = m_IO.GetAvailableVariables();
    for(const auto &i : variables)
    {
        auto it = i.second.find("Shape");
        if(it != i.second.end())
        {
            j["V"][i.first]["S"] = it->second;
        }
        it = i.second.find("Type");
        if(it != i.second.end())
        {
            j["V"][i.first]["T"] = it->second;
        }
    }
    // TODO: Add attributes
    m_MetadataJsonString = j.dump();
    m_MetadataJsonCharVector.resize(m_MetadataJsonString.size());
    std::memcpy(m_MetadataJsonCharVector.data(), m_MetadataJsonString.data(), m_MetadataJsonString.size());
}

void SscWriter::SyncMetadata()
{
    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncMetadata, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }
    size_t metadataSize = m_MetadataJsonCharVector.size();
    MPI_Bcast(&metadataSize, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

    MPI_Win win;
    MPI_Win_create(m_MetadataJsonCharVector.data(), m_MetadataJsonCharVector.size(), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    for(auto i : m_MetadataJsonCharVector)
    {
        std::cout << i ;
    }
    std::cout << std::endl;
}

void SscWriter::SyncRequests()
{

    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncRequests, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << std::endl;
    }

    size_t globalRequestSizeSrc = 0;
    size_t globalRequestSizeDst = 0;
    MPI_Allreduce(&globalRequestSizeSrc, &globalRequestSizeDst, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);

    if(m_GlobalRequestJsonString.size() < globalRequestSizeDst)
    {
        m_GlobalRequestJsonString.resize(globalRequestSizeDst);
    }

    std::vector<char> globalRequestCharVec(m_GlobalRequestJsonString.size());

    MPI_Win win;
    MPI_Win_create(globalRequestCharVec.data(), globalRequestCharVec.size(), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Get(globalRequestCharVec.data(), globalRequestCharVec.size(), MPI_CHAR, m_ReaderMasterWorldRank,0,globalRequestCharVec.size(), MPI_CHAR, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    m_GlobalRequestJsonString = std::string(globalRequestCharVec.begin(), globalRequestCharVec.end());

    auto j = nlohmann::json::parse(m_GlobalRequestJsonString);

    if(m_Verbosity >=5)
    {
        std::cout << "SscWriter::SyncRequests, World Rank " << m_WorldRank << ", Writer Rank " << m_WriterRank << " obtained reading requests: " << j.dump(4) << std::endl;
    }

    m_GlobalRequestMap.resize(m_ReaderSize);

    int s = 0;
    for(auto &rankj : j)
    {
        for(auto it = rankj.begin(); it!=rankj.end(); ++it)
        {
            auto &v = m_GlobalRequestMap[s][it.key()];
            v.type = it.value()["T"].get<std::string>();
            v.shape = it.value()["S"].get<Dims>();
            v.start = it.value()["O"].get<Dims>();
            v.count = it.value()["C"].get<Dims>();
        }
        ++s;
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
