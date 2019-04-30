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
    TAU_SCOPED_TIMER_FUNC();
    Init();
    Log(5, "WdmWriter::WdmWriter()", true, true);
}

StepStatus WdmWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER_FUNC();
    Log(5,
        "WdmWriter::BeginStep() begin. Last step " +
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
        "WdmWriter::BeginStep() end. New step " + std::to_string(m_CurrentStep),
        true, true);
    return StepStatus::OK;
}

size_t WdmWriter::CurrentStep() const { return m_CurrentStep; }

void WdmWriter::PerformPuts() {}

void WdmWriter::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    Log(5, "WdmWriter::EndStep() begin. Step " + std::to_string(m_CurrentStep),
        true, false);

    if (m_CurrentStepActive)
    {
        m_DataManSerializer.PutPack(m_DataManSerializer.GetLocalPack());
        m_DataManSerializer.AggregateMetadata(m_MPIComm);
    }

    m_DataManSerializer.Erase(m_CurrentStep - 1, true);

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
    TAU_SCOPED_TIMER_FUNC();
    MPI_Comm_rank(m_MPIComm, &m_MpiRank);
    MPI_Comm_size(m_MPIComm, &m_MpiSize);
    srand(time(NULL));
    InitParameters();
    helper::HandshakeWriter(m_MPIComm, m_AppID, m_FullAddresses, m_Name, m_Port,
                            m_Channels, m_MaxRanksPerNode, m_MaxAppsPerNode);
    InitTransports();
}

void WdmWriter::InitParameters()
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

void WdmWriter::InitTransports()
{
    TAU_SCOPED_TIMER_FUNC();
    m_Listening = true;
    for (const auto &address : m_FullAddresses)
    {
        m_ReplyThreads.emplace_back(
            std::thread(&WdmWriter::ReplyThread, this, address));
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
            int64_t stepRequested =
                reinterpret_cast<int64_t *>(request->data())[1];
            std::shared_ptr<std::vector<char>> aggMetadata = nullptr;
            int64_t stepProvided = -1;
            while (aggMetadata == nullptr)
            {
                if (stepRequested == -5) // let writer decide what to send
                {
                    aggMetadata = m_DataManSerializer.GetAggregatedMetadataPack(
                        -2, stepProvided);
                }
                else
                {
                    aggMetadata = m_DataManSerializer.GetAggregatedMetadataPack(
                        stepRequested, stepProvided);
                }
            }
            tpm.SendReply(aggMetadata);

            if (m_Verbosity >= 100)
            {
                Log(100,
                    "WdmWriter::MetadataRepThread() sending metadata "
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
                        "WdmWriter::ReplyThread received data request but "
                        "the step is already removed from buffer. Increase the"
                        "buffer size to prevent this from happening again.",
                        true, true);
                }
                else
                {
                    throw(std::runtime_error(
                        "WdmWriter::ReplyThread received data request but the "
                        "step is already removed from buffer. Increase the "
                        "buffer "
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

    remove(".staging");
    remove(std::string(m_Name + ".staging").c_str());

    Log(5, "WdmWriter::DoClose(" + m_Name + ")", true, true);
}
void WdmWriter::Log(const int level, const std::string &message, const bool mpi,
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
