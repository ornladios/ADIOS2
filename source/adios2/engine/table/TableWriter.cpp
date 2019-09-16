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
                         helper::Comm comm)
: Engine("TableWriter", io, name, mode, std::move(comm)),
  m_IsRowMajor(helper::IsRowMajor(m_IO.m_HostLanguage)),
  m_Deserializer(m_Comm, m_IsRowMajor),
  m_SubAdios(MPI_COMM_WORLD, adios2::DebugOFF),
  m_SubIO(m_SubAdios.DeclareIO("SubIO"))
{
    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();
    Init();
}

TableWriter::~TableWriter()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::~TableWriter " << m_MpiRank << std::endl;
    }
}

StepStatus TableWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::BeginStep " << m_MpiRank << std::endl;
    }
    m_SubEngine->BeginStep(mode, timeoutSeconds);
    ++m_CurrentStep;
    return StepStatus::OK;
}

size_t TableWriter::CurrentStep() const { return m_CurrentStep; }

void TableWriter::PerformPuts() {}

void TableWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::EndStep " << m_MpiRank << std::endl;
    }

    for (auto serializer : m_Serializers)
    {
        if (m_MpiSize > 1)
        {
            auto localPack = serializer->GetLocalPack();
            auto reply =
                m_SendStagingMan.Request(localPack->data(), localPack->size(),
                                         serializer->GetDestination());
            if (m_Verbosity >= 1)
            {
                std::cout << "TableWriter::EndStep Rank " << m_MpiRank
                          << " Sent a package of size " << localPack->size()
                          << " to " << serializer->GetDestination()
                          << " and received reply " << reply->data()[0]
                          << std::endl;
            }
        }
        else
        {
            auto localPack = serializer->GetLocalPack();
            m_Deserializer.PutPack(localPack);
            PutAggregatorBuffer();
        }
    }

    m_Comm.Barrier();

    m_Listening = false;
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::EndStep Rank " << m_MpiRank
                  << " Set m_Listening to false" << std::endl;
    }
    if (m_ReplyThread.joinable())
    {
        m_ReplyThread.join();
    }
    PutSubEngine(true);
    m_SubEngine->EndStep();
}

void TableWriter::ReplyThread()
{
    adios2::zmq::ZmqReqRep replier;
    replier.OpenReplier(m_AllAddresses[m_MpiRank], m_Timeout,
                        m_ReceiverBufferSize);
    while (m_Listening)
    {
        auto request = replier.ReceiveRequest();
        if (request == nullptr or request->empty())
        {
            if (m_Verbosity >= 20)
            {
                std::cout << "TableWriter::ReplyThread " << m_MpiRank
                          << " did not receive anything" << std::endl;
            }
            continue;
        }
        m_Deserializer.PutPack(request);
        format::VecPtr reply = std::make_shared<std::vector<char>>(1, 'K');
        replier.SendReply(reply);
        if (m_Verbosity >= 1)
        {
            std::cout << "TableWriter::ReplyThread " << m_MpiRank
                      << " received a package of size " << request->size()
                      << std::endl;
        }
        PutAggregatorBuffer();
        PutSubEngine();
    }
}

void TableWriter::Flush(const int transportIndex)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::Flush " << m_MpiRank << std::endl;
    }
    m_SubEngine->Flush(transportIndex);
}

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
        }
        if (key == "aggregators")
        {
            m_Aggregators = std::stoi(value);
        }
        if (key == "rowsperaggregatorbuffer")
        {
            m_RowsPerAggregatorBuffer = std::stoll(value);
        }
    }

    if (m_Aggregators > m_MpiSize)
    {
        m_Aggregators = m_MpiSize;
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
    m_Comm.Allgather(cv.data(), cv.size(), cvAll.data(), cv.size());
    for (int i = 0; i < m_MpiSize; ++i)
    {
        auto j = nlohmann::json::parse(cvAll.data() + i * 128);
        for (auto k = j.begin(); k != j.end(); ++k)
        {
            m_AllAddresses[stoull(k.key())] = k.value();
        }
    }
}

void TableWriter::InitTransports()
{
    TAU_SCOPED_TIMER_FUNC();

    m_SendStagingMan.OpenRequester(m_Timeout, 32);

    for (int i = 0; i < m_Aggregators; ++i)
    {
        auto s =
            std::make_shared<format::DataManSerializer>(m_Comm, m_IsRowMajor);
        s->NewWriterBuffer(m_SerializerBufferSize);
        s->SetDestination(m_AllAddresses[i]);
        m_Serializers.push_back(s);
    }

    if (m_MpiSize > 1)
    {
        m_Listening = true;
        m_ReplyThread = std::thread(&TableWriter::ReplyThread, this);
    }
    else
    {
        m_Listening = false;
    }

    m_SubIO.SetEngine("bp4");
    m_SubEngine = std::make_shared<adios2::Engine>(
        m_SubIO.Open(m_Name, adios2::Mode::Write));
}

void TableWriter::PutAggregatorBuffer()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutAggregatorBuffer " << m_MpiRank
                  << " begin" << std::endl;
    }

    // Get metadata map from dataman serializer
    auto metadataMap = m_Deserializer.GetFullMetadataMap();
    format::DmvVecPtr vars;
    auto currentStepIt = metadataMap.find(m_CurrentStep);
    if (currentStepIt == metadataMap.end())
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "TableWriter::PutAggregatorBuffer " << m_MpiRank
                      << " end because of empty map" << std::endl;
        }
        return;
    }
    else
    {
        vars = currentStepIt->second;
    }

    // Copy data to aggregator buffers
    for (const auto &v : *vars)
    {
        m_VarInfoMap[v.name].type = v.type;
        m_VarInfoMap[v.name].shape = v.shape;
        size_t elementSize;
        if (v.type == "")
        {
        }
#define declare_type(T)                                                        \
    else if (v.type == helper::GetType<T>()) { elementSize = sizeof(T); }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

        size_t bufferSize = std::accumulate(
            v.shape.begin() + 1, v.shape.end(),
            elementSize * m_RowsPerAggregatorBuffer, std::multiplies<size_t>());

        auto indices = WhatBufferIndices(v.start, v.count);
        for (const auto index : indices)
        {
            auto &aggBuff = m_AggregatorBuffers[index];
            auto &aggBuffFlag = m_AggregatorBufferFlags[index];
            auto aggBuffVarIt = aggBuff.find(v.name);
            if (aggBuffVarIt == aggBuff.end())
            {
                aggBuff[v.name].reserve(bufferSize);
                aggBuffFlag[v.name].resize(m_RowsPerAggregatorBuffer, false);
            }

            if (v.start.size() > 0 and v.count.size() > 0)
            {
                for (size_t i = v.start[0]; i < v.start[0] + v.count[0]; ++i)
                {
                    // TODO: This does not work correctly with putSlice yet.
                    aggBuffFlag[v.name][i % m_RowsPerAggregatorBuffer] = true;
                }
            }

            if (v.type == "")
            {
            }
#define declare_type(T)                                                        \
    else if (v.type == helper::GetType<T>())                                   \
    {                                                                          \
        helper::NdCopy<T>(                                                     \
            v.buffer->data() + v.position, v.start, v.count, true, true,       \
            reinterpret_cast<char *>(aggBuff[v.name].data()),                  \
            WhatStart(v.shape, index), WhatCount(v.shape, index), true, true); \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
        }
    }

    m_Deserializer.Erase(m_CurrentStep);

    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutAggregatorBuffer " << m_MpiRank << " end"
                  << std::endl;
    }
}

void TableWriter::PutSubEngine(bool finalPut)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutSubEngine " << m_MpiRank << " begin"
                  << std::endl;
    }

    std::unordered_map<size_t, std::vector<std::string>> toErase;
    for (const auto &indexPair : m_AggregatorBufferFlags)
    {
        for (const auto &varPair : indexPair.second)
        {
            bool ready = true;
            for (const auto flag : varPair.second)
            {
                if (flag == false)
                {
                    ready = false;
                }
            }
            if (ready or finalPut)
            {
                const Dims &shape = m_VarInfoMap[varPair.first].shape;
                const Dims start = WhatStart(shape, indexPair.first);
                Dims count = WhatCount(shape, indexPair.first);
                if (start[0] + count[0] > shape[0])
                {
                    count[0] = shape[0] - start[0];
                }
                const std::string &type = m_VarInfoMap[varPair.first].type;
                if (type == "")
                {
                }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        auto variable = m_SubIO.InquireVariable<T>(varPair.first);             \
        if (not variable)                                                      \
        {                                                                      \
            variable =                                                         \
                m_SubIO.DefineVariable<T>(varPair.first, shape, start, count); \
        }                                                                      \
        variable.SetSelection({start, count});                                 \
        m_SubEngine->Put(                                                      \
            variable,                                                          \
            reinterpret_cast<T *>(                                             \
                m_AggregatorBuffers[indexPair.first][varPair.first].data()),   \
            Mode::Sync);                                                       \
        if (m_Verbosity >= 10)                                                 \
        {                                                                      \
            std::cout << "TableWriter::PutSubEngine " << m_MpiRank             \
                      << " put start " << start[0] << " count " << count[0]    \
                      << std::endl;                                            \
        }                                                                      \
    }
                ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                toErase[indexPair.first].push_back(varPair.first);
            }
        }
    }

    // Erase aggregator buffers
    for (const auto &i : toErase)
    {
        for (const auto &j : i.second)
        {
            m_AggregatorBufferFlags[i.first].erase(j);
            m_AggregatorBuffers[i.first].erase(j);
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::PutSubEngine " << m_MpiRank << " end"
                  << std::endl;
    }
}

void TableWriter::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "TableWriter::DoClose " << m_MpiRank << std::endl;
    }
    m_SubEngine->Close();
}

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
    return row / (m_RowsPerAggregatorBuffer * m_Aggregators);
}

std::vector<int> TableWriter::WhatAggregatorIndices(const Dims &start,
                                                    const Dims &count)
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<int> ranks;
    if (start.size() > 0 and count.size() > 0)
    {
        for (size_t i = start[0]; i < start[0] + count[0]; ++i)
        {
            int rank = (i / m_RowsPerAggregatorBuffer) % m_Aggregators;
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

std::vector<std::string>
TableWriter::WhatAggregatorAddresses(const std::vector<int> &indices)
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<std::string> aggregators;
    for (const auto i : indices)
    {
        aggregators.push_back(m_AllAddresses[i]);
    }
    return aggregators;
}

std::vector<std::string> TableWriter::WhatAggregatorAddresses(const Dims &start,
                                                              const Dims &count)
{
    TAU_SCOPED_TIMER_FUNC();
    std::vector<std::string> aggregators;
    auto ranks = WhatAggregatorIndices(start, count);
    for (const auto i : ranks)
    {
        aggregators.push_back(m_AllAddresses[i]);
    }

    if (m_Verbosity >= 10)
    {
        std::cout << "TableWriter::WhatAggregators returns ";
        for (const auto i : aggregators)
        {
            std::cout << i << ", ";
        }
        std::cout << std::endl;
    }
    return aggregators;
}

Dims TableWriter::WhatStart(const Dims &shape, const size_t index)
{
    Dims start(shape.size(), 0);
    start[0] = (m_Aggregators * index + m_MpiRank) * m_RowsPerAggregatorBuffer;
    return start;
}

Dims TableWriter::WhatCount(const Dims &shape, const size_t index)
{
    Dims count = shape;
    count[0] = m_RowsPerAggregatorBuffer;
    return count;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
