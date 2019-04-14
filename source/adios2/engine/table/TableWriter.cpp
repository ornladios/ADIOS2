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
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true, helper::IsLittleEndian()),
    m_SendStagingMan(mpiComm, Mode::Read, m_Timeout, 128)
{
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    MPI_Comm_size(mpiComm, &m_MpiSize);
    Init();
}

StepStatus TableWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    ++m_CurrentStep;
    return StepStatus::OK;
}

size_t TableWriter::CurrentStep() const
{
    return m_CurrentStep;
}

void TableWriter::PerformPuts()
{
}

void TableWriter::EndStep()
{
    PerformPuts();
}

void TableWriter::Flush(const int transportIndex)
{
}

// PRIVATE

#define declare_type(T)                                                        \
    void TableWriter::DoPutSync(Variable<T> &variable, const T *data)       \
    {                                                                          \
        PutSyncCommon(variable, data);                                     \
    }                                                                          \
    void TableWriter::DoPutDeferred(Variable<T> &variable, const T *data)   \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void TableWriter::Init()
{
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
    if(ips.empty())
    {
        throw(std::runtime_error("No available network interface."));
    }
    else
    {
        j[std::to_string(m_MpiRank)] = "tcp://" + ips[0] + ":" + std::to_string(m_Port + m_MpiRank % m_MaxRanksPerNode);
    }
    std::string a = j.dump();
    std::vector<char> cv(128);
    std::vector<char> cvAll(128*m_MpiSize);
    std::memcpy(cv.data(), a.c_str(), a.size());
    MPI_Allgather(cv.data(), cv.size(), MPI_CHAR, cvAll.data(), cv.size(), MPI_CHAR, m_MPIComm);
    for(int i=0; i<m_MpiSize; ++i)
    {
        auto j = nlohmann::json::parse(cvAll.data() + i*128);
        for(auto k = j.begin(); k!=j.end(); ++k)
        {
            m_AllAddresses[stoull(k.key())] = k.value();
        }
    }

}

void TableWriter::ReplyThread()
{
    transportman::StagingMan receiveStagingMan(m_MPIComm, Mode::Write, m_Timeout, 1e9);
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

        CheckFlush();
    }
}

void TableWriter::CheckFlush()
{
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

    for (const auto &i : *vars)
    {

    }

}

void TableWriter::InitTransports()
{
    m_Listening = true;
    m_ReplyThread = std::thread(&TableWriter::ReplyThread, this);
}

void TableWriter::DoClose(const int transportIndex)
{
}

std::vector<int> TableWriter::WhichRanks(const Dims &start, const Dims &count)
{
    std::vector<int> ranks;
    if(start.size() > 0 and count.size() >0)
    {
        for(size_t i = start[0]; i<start[0]+count[0];  ++i)
        {
            int rank = WhichRank(i);
            bool exist = false;
            for(const auto &r: ranks)
            {
                if(rank == r)
                {
                    exist = true;
                }
            }
            if(not exist)
            {
                ranks.push_back(rank);
            }
        }
    }
    return ranks;
}

int TableWriter::WhichRank(size_t row)
{
    return (row / m_RowsPerRank) % m_MpiSize;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
