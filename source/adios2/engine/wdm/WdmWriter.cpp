/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WdmWriter.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "WdmWriter.h"
#include "WdmWriter.tcc"

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

WdmWriter::WdmWriter(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("WdmWriter", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian())
{
    Init();
    Log(5, "WdmWriter::WdmWriter()", true, true);
}

StepStatus WdmWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{

    Log(5, "WdmWriter::BeginStep() begin. Last step " + std::to_string(m_CurrentStep), true, true);

    if(m_QueueFullPolicy == "discard")
    {
        int64_t stepToErase = m_CurrentStep - m_QueueLimit;
        if (stepToErase >= 0)
        {
            Log(5, "WdmWriter::BeginStep() reaching max buffer steps, removing Step " + std::to_string(stepToErase), true, true);
            m_DataManSerializer.Erase(stepToErase);
        }
    }
    else if(m_QueueFullPolicy == "block")
    {
        auto startTime = std::chrono::system_clock::now();
        while (m_DataManSerializer.Steps() > m_QueueLimit)
        {
            auto nowTime = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(nowTime - startTime);
            if(duration.count() > timeoutSeconds)
            {
                Log(5, "WdmWriter::BeginStep() returned NotReady", true, true);
                return StepStatus::NotReady;
            }
        }
    }
    else
    {
        throw(std::invalid_argument("WdmWriter::ReplyThread: unknown QueueFullPolicy parameter"));
    }

    Log(5, "WdmWriter::BeginStep() after checking queue limit", true, true);

    ++m_CurrentStep;
    m_DataManSerializer.New(m_DefaultBufferSize);

    if (not m_AttributesSet)
    {
        m_DataManSerializer.PutAttributes(m_IO);
        m_AttributesSet = true;
    }

    Log(5,
        "WdmWriter::BeginStep() end. New step " + std::to_string(m_CurrentStep),
        true, true);
    return StepStatus::OK;
}

size_t WdmWriter::CurrentStep() const { return m_CurrentStep; }

void WdmWriter::PerformPuts() {}

void WdmWriter::EndStep()
{
    Log(5, "WdmWriter::EndStep() begin. Step " + std::to_string(m_CurrentStep),
        true, false);

    m_DataManSerializer.PutPack(m_DataManSerializer.GetLocalPack());
    m_DataManSerializer.AggregateMetadata(m_MPIComm);

    Log(5, "WdmWriter::EndStep() end. Step " + std::to_string(m_CurrentStep),
        true, true);
}

void WdmWriter::Flush(const int transportIndex) {}

// PRIVATE

#define declare_type(T)                                                        \
    void WdmWriter::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
    }                                                                          \
    void WdmWriter::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void WdmWriter::Init()
{
    MPI_Comm_rank(m_MPIComm, &m_MpiRank);
    MPI_Comm_size(m_MPIComm, &m_MpiSize);
    srand(time(NULL));
    InitParameters();
    Handshake();
    InitTransports();
}

void WdmWriter::InitParameters()
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
        else if(key == "queuefullpolicy")
        {
            m_QueueFullPolicy = value;
        }
    }
}

void WdmWriter::InitTransports()
{
    m_Listening = true;
    for (const auto &address : m_FullAddresses)
    {
        m_ReplyThreads.emplace_back(
            std::thread(&WdmWriter::ReplyThread, this, address));
    }
}

void WdmWriter::Handshake()
{
    auto ips = helper::AvailableIpAddresses();

    std::string ip = "127.0.0.1";

    if (ips.empty() == false)
    {
        ip = ips[0];
    }
    else
    {
        Log(1, "WdmWriter::Handshake() Cound not find any available IP "
               "address. Using local address 127.0.0.1",
            true, true);
    }

    for (int i = 0; i < m_Channels; ++i)
    {
        std::string addr =
            "tcp://" + ip + ":" +
            std::to_string(12307 + (m_MpiRank % 1000) * m_Channels + i) + "\0";
        m_FullAddresses.push_back(addr);
    }

    nlohmann::json localAddressesJson = m_FullAddresses;
    std::string localAddressesStr = localAddressesJson.dump();
    std::vector<char> localAddressesChar(64 * m_Channels, '\0');
    std::memcpy(localAddressesChar.data(), localAddressesStr.c_str(),
                localAddressesStr.size());
    std::vector<char> globalAddressesChar(64 * m_Channels * m_MpiSize, '\0');

    helper::GatherArrays(localAddressesChar.data(), 64 * m_Channels,
                         globalAddressesChar.data(), m_MPIComm);

    if (m_MpiRank == 0)
    {
        nlohmann::json globalAddressesJson;
        for (int i = 0; i < m_MpiSize; ++i)
        {
            auto j = nlohmann::json::parse(
                &globalAddressesChar[i * 64 * m_Channels]);
            for (auto &i : j)
            {
                globalAddressesJson.push_back(i);
            }
        }

        std::string globalAddressesStr = globalAddressesJson.dump();

        transport::FileFStream lockstream(m_MPIComm, m_DebugMode);
        lockstream.Open(".StagingHandshakeLock", Mode::Write);

        transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
        ipstream.Open(".StagingHandshake", Mode::Write);
        ipstream.Write(globalAddressesStr.data(), globalAddressesStr.size());
        ipstream.Close();

        lockstream.Close();
        remove(".StagingHandshakeLock");
    }
}

void WdmWriter::ReplyThread(const std::string &address)
{
    transportman::StagingMan tpm(m_MPIComm, Mode::Write, m_Timeout, 1e9);
    tpm.OpenTransport(address);
    while (m_Listening)
    {
        auto request = tpm.ReceiveRequest();
        if (request == nullptr)
        {
            continue;
        }
        if (request->size() == 2 * sizeof(int64_t))
        {
            int64_t reader_id = reinterpret_cast<int64_t *>(request->data())[0];
            int64_t step = reinterpret_cast<int64_t *>(request->data())[1];
            std::shared_ptr<std::vector<char>> aggMetadata = nullptr;
            while (aggMetadata == nullptr)
            {
                if (step == -5) // let writer decide what to send
                {
                    if (m_QueueFullPolicy == "discard")
                    {
                        aggMetadata =
                            m_DataManSerializer.GetAggregatedMetadataPack(-2);
                    }
                    else if (m_QueueFullPolicy == "block")
                    {
                        aggMetadata =
                            m_DataManSerializer.GetAggregatedMetadataPack(-4);
                    }
                    else
                    {
                        throw(std::invalid_argument("WdmWriter::ReplyThread: unknown QueueFullPolicy parameter"));
                    }
                }
                else
                {
                    aggMetadata =
                        m_DataManSerializer.GetAggregatedMetadataPack(step);
                }
            }
            tpm.SendReply(aggMetadata);

            if (m_Verbosity >= 100)
            {
                Log(100, "WdmWriter::MetadataRepThread() sending metadata "
                         "pack, size =  " +
                             std::to_string(aggMetadata->size()),
                    true, true);
                std::cout << nlohmann::json::parse(*aggMetadata).dump(4)
                          << std::endl;
            }
        }
        else if (request->size() > 16)
        {
            size_t step;
            auto reply = m_DataManSerializer.GenerateReply(*request, step);
            tpm.SendReply(reply);
            if (reply->size() <= 16)
            {
                if (m_Tolerance)
                {
                    Log(1, "WdmWriter::ReplyThread received data request but "
                           "the step is already removed from buffer. Increase the"
                           "buffer size to prevent this from happening again.",
                        true, true);
                }
                else
                {
                    throw(std::runtime_error(
                        "WdmWriter::ReplyThread received data request but the "
                        "step is already removed from buffer. Increase the buffer "
                        "size to prevent this from happening again."));
                }
            }
        }
    }
}

void WdmWriter::DoClose(const int transportIndex)
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

    remove(".StagingHandshake");

    Log(5, "WdmWriter::DoClose(" + m_Name + ")", true, true);
}
void WdmWriter::Log(const int level, const std::string &message, const bool mpi,
                    const bool endline)
{
    if (m_Verbosity >= level)
    {
        if (mpi)
        {
            std::cout << "[Rank " << m_MpiRank << "] ";
        }
        std::cout << message;
        if (endline)
        {
            std::cout << std::endl;
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
