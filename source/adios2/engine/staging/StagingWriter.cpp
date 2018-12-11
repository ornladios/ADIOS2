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
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Writer " << m_MpiRank << " Open(" << m_Name
                  << ")." << std::endl;
    }
}

StepStatus StagingWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    ++m_CurrentStep;

    int64_t stepToErase = m_CurrentStep - m_MaxBufferSteps;
    {
        std::lock_guard<std::mutex> l(m_Mutex);
        if (m_ProtectedSteps.empty() == false)
        {
            if (m_ProtectedSteps.front() <= stepToErase)
            {
                stepToErase = m_ProtectedSteps.front() - 1;
            }
        }
    }
    if (stepToErase >= 0)
    {
        m_DataManSerializer.Erase(stepToErase, true);
    }

    if (m_DataManSerializer.Steps() >= m_MaxBufferSteps)
    {
        m_IsActive = false;
        if (m_Verbosity >= 5)
        {
            std::cout << "Staging Writer " << m_MpiRank
                      << "   BeginStep() buffer full skip " << m_CurrentStep
                      << "\n";
        }
        return StepStatus::NotReady;
    }
    else
    {
        m_IsActive = true;
    }

    m_DataManSerializer.New(m_DefaultBufferSize);
    m_MaxStep = m_CurrentStep;

    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Writer " << m_MpiRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }
    return StepStatus::OK;
}

size_t StagingWriter::CurrentStep() const
{
    if (m_Verbosity == 5)
    {
        std::cout << "Staging Writer " << m_MpiRank
                  << "   CurrentStep() returns " << m_CurrentStep << "\n";
    }
    return m_CurrentStep;
}

void StagingWriter::PerformPuts() {}

void StagingWriter::EndStep()
{
    if (m_IsActive)
    {
        auto aggMetadata = m_DataManSerializer.GetAggregatedMetadata(m_MPIComm);
        if (m_MpiRank == 0)
        {
            std::lock_guard<std::mutex> l(m_Mutex);
            m_LockedAggregatedMetadata = aggMetadata;
            m_LockedStep = m_CurrentStep;
        }
        m_DataManSerializer.PutPack(m_DataManSerializer.GetLocalPack());
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Writer " << m_MpiRank << "   EndStep()\n";
    }
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
    m_DataRepThread =
        std::make_shared<std::thread>(&StagingWriter::DataRepThread, this);
    if (m_MpiRank == 0)
    {
        m_MetadataRepThread = std::make_shared<std::thread>(
            &StagingWriter::MetadataRepThread, this);
    }
}

void StagingWriter::Handshake()
{
    auto ips = helper::AvailableIpAddresses();

    std::string ip;
    if (ips.empty() == false)
    {
        ip = ips[0];
    }
    else
    {
        std::cout << "Cound not find any available IP address. Using local "
                     "address 127.0.0.1"
                  << std::endl;
        ip = "127.0.0.1";
    }

    m_FullMetadataAddress = "tcp://" + ip + ":" + std::to_string(12306) + "\0";
    m_FullDataAddress =
        "tcp://" + ip + ":" + std::to_string(12307 + m_MpiRank) + "\0";

    transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
    ipstream.Open(".StagingHandshake", Mode::Write);
    ipstream.Write(m_FullMetadataAddress.data(), m_FullMetadataAddress.size());
    ipstream.Close();
}

void StagingWriter::DataRepThread()
{
    transportman::StagingMan tpm(m_MPIComm, Mode::Write, m_Timeout, 1e9);
    tpm.OpenTransport(m_FullDataAddress);
    while (m_Listening)
    {
        auto request = tpm.ReceiveRequest();
        if (request->size() > 0)
        {
            auto reply = m_DataManSerializer.GenerateReply(request, true);
            tpm.SendReply(reply);
            {
                std::lock_guard<std::mutex> l(m_Mutex);
                while (m_ProtectedSteps.empty() == false)
                {
                    if (m_ProtectedSteps.front() + m_MaxBufferSteps <
                        m_LockedStep)
                    {
                        m_ProtectedSteps.pop();
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
}

void StagingWriter::MetadataRepThread()
{
    transportman::StagingMan tpm(m_MPIComm, Mode::Write, m_Timeout, 1e6);
    tpm.OpenTransport(m_FullMetadataAddress);
    while (m_Listening)
    {
        auto request = tpm.ReceiveRequest();
        if (request->size() > 0)
        {
            std::shared_ptr<std::vector<char>> aggMetadata;
            int64_t stepToErase;
            {
                std::lock_guard<std::mutex> l(m_Mutex);
                aggMetadata = m_LockedAggregatedMetadata;
                if (aggMetadata != nullptr)
                {
                    m_ProtectedSteps.push(m_LockedStep);
                    while (m_ProtectedSteps.empty() == false)
                    {
                        if (m_ProtectedSteps.front() + m_MaxBufferSteps <
                            m_LockedStep)
                        {
                            m_ProtectedSteps.pop();
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                m_LockedAggregatedMetadata = nullptr;
                stepToErase = m_LockedStep - 1;
            }
            if (aggMetadata != nullptr)
            {
                tpm.SendReply(aggMetadata);
                if (stepToErase >= 0)
                {
                    m_DataManSerializer.Erase(stepToErase, true);
                }
            }
            else
            {
                aggMetadata = std::make_shared<std::vector<char>>(1, 'N');
                tpm.SendReply(aggMetadata);
            }

            if (m_Verbosity >= 100)
            {
                if (m_MpiRank == 0)
                {
                    std::cout << "StagingWriter::MetadataRepThread Cbor data, "
                                 "size =  "
                              << aggMetadata->size() << std::endl;
                    std::cout << "========================" << std::endl;
                    for (auto i : *aggMetadata)
                    {
                        std::cout << i;
                    }
                    std::cout << std::endl
                              << "========================" << std::endl;
                }
            }
        }
    }
}

void StagingWriter::DoClose(const int transportIndex)
{
    m_Listening = false;
    if (m_DataRepThread != nullptr)
    {
        if (m_DataRepThread->joinable())
        {
            m_DataRepThread->join();
        }
        m_DataRepThread = nullptr;
    }
    if (m_MpiRank == 0)
    {
        if (m_MetadataRepThread != nullptr)
        {
            if (m_MetadataRepThread->joinable())
            {
                m_MetadataRepThread->join();
            }
            m_MetadataRepThread = nullptr;
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Writer " << m_MpiRank << " Close(" << m_Name
                  << ")\n";
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
