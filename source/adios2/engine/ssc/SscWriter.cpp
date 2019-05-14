/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "SscWriter.h"
#include "SscWriter.tcc"

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

SscWriter::SscWriter(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("SscWriter", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian(), mpiComm)
{
    TAU_SCOPED_TIMER_FUNC();
    Init();
    Log(5, "SscWriter::SscWriter()", true, true);
}

StepStatus SscWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();
    Log(5,
        "SscWriter::BeginStep() begin. Last step " +
            std::to_string(m_CurrentStep),
        true, true);

    ++m_CurrentStep;
    if (m_CurrentStep % m_StepsPerAggregation == 0)
    {
        m_CurrentStepActive = true;
        m_DataManSerializer.New(m_DefaultBufferSize);
        if (not m_AttributesSet)
        {
            m_DataManSerializer.PutAttributes(m_IO);
            m_AttributesSet = true;
        }
    }
    else
    {
        m_CurrentStepActive = false;
    }

    Log(5,
        "SscWriter::BeginStep() end. New step " + std::to_string(m_CurrentStep),
        true, true);
    return StepStatus::OK;
}

size_t SscWriter::CurrentStep() const { return m_CurrentStep; }

void SscWriter::PerformPuts() {}

void SscWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    Log(5, "SscWriter::EndStep() begin. Step " + std::to_string(m_CurrentStep),
        true, false);

    if (m_CurrentStepActive)
    {
        m_DataManSerializer.PutPack(m_DataManSerializer.GetLocalPack());
        m_DataManSerializer.AggregateMetadata();
    }

    if (m_CurrentStep > 5)
    {
        m_DataManSerializer.Erase(m_CurrentStep - 5, true);
    }

    Log(5, "SscWriter::EndStep() end. Step " + std::to_string(m_CurrentStep),
        true, true);
}

void SscWriter::Flush(const int transportIndex) {}

// PRIVATE

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

void SscWriter::Init()
{
    TAU_SCOPED_TIMER_FUNC();
    MPI_Comm_rank(m_MPIComm, &m_MpiRank);
    MPI_Comm_size(m_MPIComm, &m_MpiSize);
    srand(time(NULL));
    InitParameters();
    helper::HandshakeWriter(m_MPIComm, m_AppID, m_FullAddresses, m_Name, "ssc",
                            m_Port, m_Channels, m_MaxRanksPerNode,
                            m_MaxAppsPerNode);
    InitTransports();
}

void SscWriter::InitParameters()
{
    TAU_SCOPED_TIMER_FUNC();
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
        else if (key == "port")
        {
            m_Port = std::stoi(value);
        }
        else if (key == "stepsperaggregation")
        {
            m_StepsPerAggregation = std::stoi(value);
        }
    }
}

void SscWriter::InitTransports()
{
    TAU_SCOPED_TIMER_FUNC();
    m_Listening = true;
    for (const auto &address : m_FullAddresses)
    {
        m_ReplyThreads.emplace_back(
            std::thread(&SscWriter::ReplyThread, this, address));
    }
}

void SscWriter::ReplyThread(const std::string &address)
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
            int64_t stepRequested =
                reinterpret_cast<int64_t *>(request->data())[1];
            std::shared_ptr<std::vector<char>> aggMetadata = nullptr;
            int64_t stepProvided = -1;
            while (aggMetadata == nullptr)
            {
                if (stepRequested == -5) // let writer decide what to send
                {
                    aggMetadata = m_DataManSerializer.GetAggregatedMetadataPack(
                        -2, stepProvided, m_AppID);
                }
                else
                {
                    aggMetadata = m_DataManSerializer.GetAggregatedMetadataPack(
                        stepRequested, stepProvided, m_AppID);
                }
            }
            tpm.SendReply(aggMetadata);

            if (m_Verbosity >= 100)
            {
                Log(100,
                    "SscWriter::MetadataRepThread() sending metadata "
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
            m_CompressionParamsMutex.lock();
            std::unordered_map<std::string, Params> p = m_CompressionParams;
            m_CompressionParamsMutex.unlock();
            auto reply = m_DataManSerializer.GenerateReply(*request, step, p);
            tpm.SendReply(reply);
            if (reply->size() <= 16)
            {
                if (m_Tolerance)
                {
                    Log(1,
                        "SscWriter::ReplyThread received data request but "
                        "the step is already removed from buffer. Increase the"
                        "buffer size to prevent this from happening again.",
                        true, true);
                }
                else
                {
                    throw(std::runtime_error(
                        "SscWriter::ReplyThread received data request but the "
                        "step is already removed from buffer. Increase the "
                        "buffer "
                        "size to prevent this from happening again."));
                }
            }
        }
    }
}

void SscWriter::DoClose(const int transportIndex)
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

    remove(".staging");
    remove(std::string(m_Name + ".staging").c_str());

    Log(5, "SscWriter::DoClose(" + m_Name + ")", true, true);
}
void SscWriter::Log(const int level, const std::string &message, const bool mpi,
                    const bool endline)
{
    TAU_SCOPED_TIMER_FUNC();
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
