/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingWriter.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "StagingWriter.h"
#include "StagingWriter.tcc"

#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <iostream>

#include <zmq.h>

namespace adios2
{
namespace core
{
namespace engine
{

StagingWriter::StagingWriter(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("StagingWriter", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian())
{
    m_EndMessage = " in call to StagingWriter " + m_Name + " Open\n";
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    MPI_Comm_size(mpiComm, &m_MpiSize);
    Init();
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Writer " << m_MpiRank << " Open(" << m_Name
                  << ")." << std::endl;
    }
}

StepStatus StagingWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " BeginStep() start. Last step " + std::to_string(m_CurrentStep));

    ++m_CurrentStep;

    int64_t stepToErase = m_CurrentStep - m_MaxBufferSteps;

    if(stepToErase > 0)
    {
        m_DataManSerializer.Erase(stepToErase);
    }

    if (m_DataManSerializer.Steps() >= m_MaxBufferSteps)
    {
        m_IsActive = false;
        Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " BeginStep() buffer full skipping step " + std::to_string(m_CurrentStep));
        return StepStatus::NotReady;
    }
    else
    {
        m_IsActive = true;
    }

    m_DataManSerializer.New(m_DefaultBufferSize);

    Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " BeginStep() end. New step " + std::to_string(m_CurrentStep));
    return StepStatus::OK;
}

size_t StagingWriter::CurrentStep() const
{
    return m_CurrentStep;
}

void StagingWriter::PerformPuts() {}

void StagingWriter::EndStep()
{
    Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " EndStep() start. Step " + std::to_string(m_CurrentStep));

    if (m_IsActive)
    {
        Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " EndStep() Step " + std::to_string(m_CurrentStep) + " is active");
        auto aggMetadata = m_DataManSerializer.GetAggregatedMetadata(m_MPIComm);
        {
            std::lock_guard<std::mutex> l(m_LockedAggregatedMetadataMutex);
            m_LockedAggregatedMetadata.first = m_CurrentStep;
            m_LockedAggregatedMetadata.second = aggMetadata;
        }
        m_DataManSerializer.PutPack(m_DataManSerializer.GetLocalPack());
    }
    else
    {
        Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " EndStep() Step " + std::to_string(m_CurrentStep) + " is not active");
    }

    Log(5, "Staging Writer " + std::to_string( m_MpiRank) + " EndStep() end. Step " + std::to_string(m_CurrentStep));
}

void StagingWriter::Flush(const int transportIndex)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Writer " << m_MpiRank << "   Flush()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void StagingWriter::DoPutSync(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
    }                                                                          \
    void StagingWriter::DoPutDeferred(Variable<T> &variable, const T *data)    \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void StagingWriter::Init()
{
    srand (time(NULL));
    InitParameters();
    Handshake();
    InitTransports();
}

void StagingWriter::InitParameters()
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
}

void StagingWriter::InitTransports()
{
    m_Listening = true;
    for(const auto &address : m_FullAddresses)
    {
        m_ReplyThreads.emplace_back( std::thread(&StagingWriter::ReplyThread, this, address));
    }
}

void StagingWriter::Handshake()
{
    auto ips = helper::AvailableIpAddresses();

    std::string ip = "127.0.0.1";

    if (ips.empty() == false)
    {
        ip = ips[0];
    }
    else
    {
        Log(1, "Staging Writer " + std::to_string( m_MpiRank) + "Cound not find any available IP address. Using local address 127.0.0.1");
    }

    for(int i=0; i<m_Channels; ++i)
    {
        std::string addr = "tcp://" + ip + ":" + std::to_string(12307 + (m_MpiRank % 1000)*m_Channels + i) + "\0";
        m_FullAddresses.push_back(addr);
    }

    nlohmann::json localAddressesJson = m_FullAddresses;
    std::string localAddressesStr = localAddressesJson.dump();
    std::vector<char> localAddressesChar(64*m_Channels, '\0');
    std::memcpy(localAddressesChar.data(), localAddressesStr.c_str(), localAddressesStr.size());
    std::vector<char> globalAddressesChar(64*m_Channels*m_MpiSize, '\0');

    helper::GatherArrays(localAddressesChar.data(), 64*m_Channels, globalAddressesChar.data(), m_MPIComm);

    if(m_MpiRank == 0)
    {
        nlohmann::json globalAddressesJson;
        for(int i=0; i<m_MpiSize; ++i)
        {
            auto j = nlohmann::json::parse(&globalAddressesChar[i*64*m_Channels]);
            for(auto &i : j)
            {
                globalAddressesJson.push_back(i);
            }
        }

        std::string globalAddressesStr = globalAddressesJson.dump();

        transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
        ipstream.Open(".StagingHandshake", Mode::Write);
        ipstream.Write(globalAddressesStr.data(), globalAddressesStr.size());
        ipstream.Close();
    }
}

void StagingWriter::ReplyThread(std::string address)
{
    transportman::StagingMan tpm(m_MPIComm, Mode::Write, m_Timeout, 1e9);
    tpm.OpenTransport(address);
    while (m_Listening)
    {
        auto request = tpm.ReceiveRequest();
        if (request->size() == 1)
        {
            std::shared_ptr<std::vector<char>> aggMetadata = nullptr;
            int64_t aggStep;

            while(aggMetadata == nullptr)
            {
                std::lock_guard<std::mutex> l(m_LockedAggregatedMetadataMutex);
                aggMetadata = m_LockedAggregatedMetadata.second;
                aggStep =  m_LockedAggregatedMetadata.first;
                m_LockedAggregatedMetadata.second = nullptr;
            }
            tpm.SendReply(aggMetadata);

            if (m_Verbosity >= 100)
            {
                if (m_MpiRank == 0)
                {
                    std::cout << "StagingWriter::MetadataRepThread metadata pack, size =  " << aggMetadata->size() << std::endl;
                    std::cout << "========================" << std::endl;
                    for (auto i : *aggMetadata)
                    {
                        std::cout << i;
                    }
                    std::cout << std::endl << "========================" << std::endl;
                }
            }
        }
        else if (request->size() > 1)
        {
            size_t step;
            auto reply = m_DataManSerializer.GenerateReply(*request, step);
            if(reply->size() <= 16)
            {
                throw(std::runtime_error("StagingWriter::DataRepThread received request but data step is already erased."));
            }
            tpm.SendReply(reply);
        }
    }
}

void StagingWriter::DoClose(const int transportIndex)
{
    m_Listening = false;
    for (auto &i : m_ReplyThreads)
    {
        if (i.joinable())
        {
            i.join();
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Writer " << m_MpiRank << " Close(" << m_Name << ")" << std::endl;
    }
}
void StagingWriter::Log(const int level, const std::string &message)
{
    if (m_Verbosity >= level)
    {
        std::cout << message << std::endl;
    }
}


} // end namespace engine
} // end namespace core
} // end namespace adios2
