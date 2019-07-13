/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#include "TableWriter.h"
#include "TableWriter.tcc"

#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

TableWriter::TableWriter(IO &io, const std::string &name, const Mode mode,
                         MPI_Comm mpiComm)
: Engine("TableWriter", io, name, mode, mpiComm),
  m_SubEngine(io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian()),
  m_SendStagingMan(mpiComm, Mode::Read, m_Timeout, 128)
{
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    MPI_Comm_size(mpiComm, &m_MpiSize);
    Init();
}

TableWriter::~TableWriter()
{
    m_Listening = false;
    if (m_ReplyThread.joinable())
    {
        m_ReplyThread.join();
    }
}

StepStatus TableWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();
    m_SubEngine.BeginStep(mode, timeoutSeconds);
    ++m_CurrentStep;
    return StepStatus::OK;
}

size_t TableWriter::CurrentStep() const { return m_CurrentStep; }

void TableWriter::PerformPuts() {}

void TableWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    PerformPuts();
    m_SubEngine.EndStep();
}

void TableWriter::Flush(const int transportIndex) {}

// PRIVATE

#define declare_type(T)                                                        \
    void TableWriter::DoPutSync(Variable<T> &variable, const T *data)          \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
    }                                                                          \
    void TableWriter::DoPutDeferred(Variable<T> &variable, const T *data)      \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void TableWriter::Init()
{
    TAU_SCOPED_TIMER_FUNC();
    InitParameters();
    InitTransports();
}

void TableWriter::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Verbosity = std::stoi(value);
            if (m_DebugMode)
            {
                if (m_Verbosity < 0 || m_Verbosity > 5)
                    throw std::invalid_argument(
                        "ERROR: Method verbose argument must be an "
                        "integer in the range [0,5], in call to "
                        "Open or Engine constructor\n");
            }
        }
    }

    auto ips = helper::AvailableIpAddresses();
    nlohmann::json j;
    if (ips.empty())
    {
        throw(std::runtime_error("No available network interface."));
    }
    else
    {
        j[std::to_string(m_MpiRank)] =
            "tcp://" + ips[0] + ":" +
            std::to_string(m_Port + m_MpiRank % m_MaxRanksPerNode);
    }
    std::string a = j.dump();
    std::vector<char> cv(128);
    std::vector<char> cvAll(128 * m_MpiSize);
    std::memcpy(cv.data(), a.c_str(), a.size());
    MPI_Allgather(cv.data(), cv.size(), MPI_CHAR, cvAll.data(), cv.size(),
                  MPI_CHAR, m_MPIComm);
    for (int i = 0; i < m_MpiSize; ++i)
    {
        auto j = nlohmann::json::parse(cvAll.data() + i * 128);
        for (auto k = j.begin(); k != j.end(); ++k)
        {
            m_AllAddresses[stoull(k.key())] = k.value();
        }
    }
}

void TableWriter::ReplyThread()
{

    int times = 0;

    transportman::StagingMan receiveStagingMan(m_MPIComm, Mode::Write,
                                               m_Timeout, 1e9);
    receiveStagingMan.OpenTransport(m_AllAddresses[m_MpiRank]);
    while (m_Listening)
    {
        auto request = receiveStagingMan.ReceiveRequest();
        if (request == nullptr)
        {
            continue;
        }
        if (request->empty())
        {
            std::cout << "request->empty\n";
            continue;
        }
        m_DataManSerializer.PutPack(request);
        format::VecPtr reply = std::make_shared<std::vector<char>>();
        reply->resize(1);
        receiveStagingMan.SendReply(reply);

        ++times;
        if (times == m_PutSubEngineFrequency)
        {
            PutSubEngine();
            times = 0;
        }
    }
}

void TableWriter::PutSubEngine()
{
    TAU_SCOPED_TIMER_FUNC();
    auto metadataMap = m_DataManSerializer.GetMetaData();

    format::DmvVecPtr vars;

    auto currentStepIt = metadataMap.find(m_CurrentStep);
    if (currentStepIt == metadataMap.end())
    {
        throw("empty metadata map");
    }
    else
    {
        vars = currentStepIt->second;
    }

    for (const auto &v : *vars)
    {

        auto cmi = m_CountMap.find(v.name);
        if (cmi == m_CountMap.end())
        {
            m_CountMap[v.name] = v.count;
            m_CountMap[v.name][0] = 1;
        }

        size_t elementSize;
        if (v.type == "compound")
        {
            throw("Compound type is not supported yet.");
        }
#define declare_type(T)                                                        \
    else if (v.type == helper::GetType<T>()) { elementSize = sizeof(T); }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

        auto indices = WhatBufferIndices(v.start, v.count);
        size_t bufferSize = std::accumulate(v.shape.begin() + 1, v.shape.end(),
                                            elementSize * m_RowsPerRank,
                                            std::multiplies<size_t>());

        for (const auto index : indices)
        {
            auto aggBuffIt = m_AggregatorBuffers[index].find(v.name);
            if (aggBuffIt == m_AggregatorBuffers[index].end())
            {
                m_AggregatorBuffers[index][v.name].reserve(bufferSize);
                m_AggregatorBufferFlags[index][v.name].resize(m_RowsPerRank,
                                                              false);
            }
        }

        if (v.start.size() > 0 and v.count.size() > 0)
        {
            for (size_t i = v.start[0]; i < v.start[0] + v.count[0]; ++i)
            {
                m_AggregatorBufferFlags[WhatBufferIndex(i)][v.name]
                                       [i % m_RowsPerRank] = true;
            }
        }

        // put into aggregator buffer
    }

    std::unordered_map<size_t, std::vector<std::string>> toErase;
    for (auto indexPair = m_AggregatorBufferFlags.begin();
         indexPair != m_AggregatorBufferFlags.end(); ++indexPair)
    {
        for (auto varPair = indexPair->second.begin();
             varPair != indexPair->second.end(); ++varPair)
        {
            bool ready = true;
            for (const auto flag : varPair->second)
            {
                if (flag == false)
                {
                    ready = false;
                }
            }
            if (ready)
            {
                // put sub engine
                std::cout << " =========== ready to put into sub engine, "
                          << indexPair->first << ", " << varPair->first
                          << std::endl;
                toErase[indexPair->first].push_back(varPair->first);
            }
        }
    }
    for (const auto &i : toErase)
    {
        for (const auto &j : i.second)
        {
            m_AggregatorBufferFlags[i.first].erase(j);
            m_AggregatorBuffers[i.first].erase(j);
        }
    }

    m_DataManSerializer.Erase(m_CurrentStep);
    std::cout << "end PutSubEngine" << std::endl;
}

void TableWriter::InitTransports()
{
    TAU_SCOPED_TIMER_FUNC();
    m_Listening = true;
    m_ReplyThread = std::thread(&TableWriter::ReplyThread, this);
}

void TableWriter::DoClose(const int transportIndex) {}

std::vector<size_t> TableWriter::WhatBufferIndices(const Dims &start,
                                                   const Dims &count)
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<size_t> indices;
    if (start.size() > 0 and count.size() > 0)
    {
        for (size_t i = start[0]; i < start[0] + count[0]; ++i)
        {
            size_t index = WhatBufferIndex(i);
            bool exist = false;
            for (const auto &n : indices)
            {
                if (index == n)
                {
                    exist = true;
                }
            }
            if (not exist)
            {
                indices.push_back(index);
            }
        }
    }
    return indices;
}

size_t TableWriter::WhatBufferIndex(const size_t row)
{
    return row / (m_RowsPerRank * m_MpiSize);
}

std::vector<int> TableWriter::WhatRanks(const Dims &start, const Dims &count)
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<int> ranks;
    if (start.size() > 0 and count.size() > 0)
    {
        for (size_t i = start[0]; i < start[0] + count[0]; ++i)
        {
            int rank = WhatRank(i);
            bool exist = false;
            for (const auto &r : ranks)
            {
                if (rank == r)
                {
                    exist = true;
                }
            }
            if (not exist)
            {
                ranks.push_back(rank);
            }
        }
    }
    return ranks;
}

int TableWriter::WhatRank(const size_t row)
{
    return (row / m_RowsPerRank) % m_MpiSize;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
