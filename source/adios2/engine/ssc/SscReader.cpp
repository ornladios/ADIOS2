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
    m_ReaderRank = m_Comm.Rank();
    SyncMetadata();
    DeserializeMetadata();
}

SscReader::~SscReader() { TAU_SCOPED_TIMER_FUNC(); }

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();
    return StepStatus::OK;
}

void SscReader::PerformGets() { TAU_SCOPED_TIMER_FUNC(); }

size_t SscReader::CurrentStep() const
{
    TAU_SCOPED_TIMER_FUNC();
    return m_CurrentStep;
}

void SscReader::EndStep() { TAU_SCOPED_TIMER_FUNC(); }

// PRIVATE

void SscReader::SyncMetadata()
{
    if(m_Verbosity >=5)
    {
        std::cout << "SscReader::SyncMetadata, World Rank " << m_WorldRank << ", Reader Rank " << m_ReaderRank << std::endl;
    }

    size_t metadataSize = m_MetadataJsonCharVector.size();
    MPI_Bcast(&metadataSize, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    m_MetadataJsonCharVector.resize(metadataSize);

    MPI_Win win;
    MPI_Win_create(NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);
    MPI_Get(m_MetadataJsonCharVector.data(), m_MetadataJsonCharVector.size(), MPI_CHAR, 0,0,m_MetadataJsonCharVector.size(), MPI_CHAR, win);
    MPI_Win_fence(0, win);
    MPI_Win_free(&win);

    for(auto i : m_MetadataJsonCharVector)
    {
        std::cout << i ;
    }
    std::cout << std::endl;
}

void SscReader::DeserializeMetadata()
{
    nlohmann::json j = nlohmann::json::parse(m_MetadataJsonCharVector);
    auto itAV = j.find("A");
    if(itAV != j.end())
    {
        // TODO: Add attributes
    }
    itAV = j.find("V");
    if(itAV != j.end())
    {
        std::cout << itAV->dump(4) << std::endl;
        for(auto itVar = itAV->begin(); itVar != itAV->end(); ++itVar)
        {
            std::string shapeStr;
            std::string type;
            auto itST = itVar.value().find("S");
            if(itST != itVar.value().end())
            {
                shapeStr = *itST;
            }
            else{
                throw(std::runtime_error("metadata corrupted"));
            }
            itST = itVar.value().find("T");
            if(itST != itVar.value().end())
            {
                type = *itST;
            }
            else{
                throw(std::runtime_error("metadata corrupted"));
            }
            Dims shape;
            try
            {
                shape = helper::StringToDims(shapeStr);
            }
            catch(...)
            {
                throw(std::runtime_error("metadata corrupted"));
            }
            Dims start = Dims(shape.size(), 0);
            if(type.empty())
            {
                throw(std::runtime_error("unknown data type"));
            }
#define declare_type(T)                                                        \
            else if (type == helper::GetType<T>())                                   \
            {                                                                          \
                m_IO.DefineVariable<T>(itVar.key(), shape, start, shape);\
            }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else {
                throw(std::runtime_error("unknown data type"));
            }
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

void SscReader::DoClose(const int transportIndex) { TAU_SCOPED_TIMER_FUNC(); }

} // end namespace engine
} // end namespace core
} // end namespace adios2
