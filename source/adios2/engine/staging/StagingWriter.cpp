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
    Log(5, "StagingWriter::StagingWriter()", true, true);
    Init();
}

StepStatus StagingWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    Log(5, "StagingWriter::BeginStep() begin. Last step " + std::to_string(m_CurrentStep), true, true);

    ++m_CurrentStep;

    int64_t stepToErase = m_CurrentStep - m_MaxBufferSteps - 1;

    if(stepToErase > 0)
    {
        m_DataManSerializer.Erase(stepToErase);
    }

    if (m_DataManSerializer.Steps() >= m_MaxBufferSteps)
    {
        m_IsActive = false;
        Log(5, "StagingWriter::BeginStep() buffer full skipping step " + std::to_string(m_CurrentStep), true, true);
        return StepStatus::NotReady;
    }
    else
    {
        m_IsActive = true;
    }

    m_DataManSerializer.New(m_DefaultBufferSize);
    m_DataManSerializer.PutAttributes(m_IO, m_MpiRank);

    Log(5, "StagingWriter::BeginStep() end. New step " + std::to_string(m_CurrentStep), true,true);
    return StepStatus::OK;
}

size_t StagingWriter::CurrentStep() const
{
    return m_CurrentStep;
}

void StagingWriter::PerformPuts() {}

void StagingWriter::EndStep()
{
    Log(5, "StagingWriter::EndStep() begin. Step " + std::to_string(m_CurrentStep), true, false);

    if (m_IsActive)
    {
        Log(5, " is active", false, true);
        m_DataManSerializer.PutPack(m_DataManSerializer.GetLocalPack());
        auto aggMetadata = m_DataManSerializer.GetAggregatedMetadata(m_MPIComm);
        {
            // TODO: this is subject to test at scale, without a barrier there is possibility that different ranks reply with different newest steps to the readers, because on some ranks a request may be received before this mutex is locked, while on other ranks the request for the current step may be received after this locked metadata is updated.
            std::lock_guard<std::mutex> l(m_LockedAggregatedMetadataMutex);
            m_LockedAggregatedMetadata.first = m_CurrentStep;
            m_LockedAggregatedMetadata.second = aggMetadata;
        }
    }
    else
    {
        Log(5, " is not active", false, true);
    }

    Log(5, "StagingWriter::EndStep() end. Step " + std::to_string(m_CurrentStep), true, true);
}

void StagingWriter::Flush(const int transportIndex)
{
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
    MPI_Comm_rank(m_MPIComm, &m_MpiRank);
    MPI_Comm_size(m_MPIComm, &m_MpiSize);
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
        Log(1, "StagingWriter::Handshake() Cound not find any available IP address. Using local address 127.0.0.1", true, true);
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
            m_DataManSerializer.ProtectStep(aggStep);
            tpm.SendReply(aggMetadata);

            if (m_Verbosity >= 100)
            {
                Log(100,"StagingWriter::MetadataRepThread() sending metadata pack, size =  " + std::to_string( aggMetadata->size()), true, true) ;
                std::cout << nlohmann::json::parse(*aggMetadata).dump(4) << std::endl;
            }
        }
        else if (request->size() > 1)
        {
            size_t step;
            auto reply = m_DataManSerializer.GenerateReply(*request, step);
            tpm.SendReply(reply);
            if(reply->size() <= 16)
            {
                throw(std::runtime_error("sending empty package"));
            }
        }
    }
}

void StagingWriter::DoClose(const int transportIndex)
{
    MPI_Barrier(m_MPIComm);
    m_Listening = false;
    for (auto &i : m_ReplyThreads)
    {
        if (i.joinable())
        {
            i.join();
        }
    }

    Log(5, "StagingWriter::DoClose(" + m_Name + ")", true, true);

}
void StagingWriter::Log(const int level, const std::string &message, const bool mpi, const bool endline)
{
    if (m_Verbosity >= level)
    {
        if(mpi)
        {
            std::cout << "[Rank " << m_MpiRank << "] ";
        }
        std::cout << message;
        if(endline)
        {
            std::cout << std::endl;
        }
    }
}


} // end namespace engine
} // end namespace core
} // end namespace adios2
