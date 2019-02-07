/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WdmReader.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "WdmReader.h"
#include "WdmReader.tcc"

#include "adios2/helper/adiosFunctions.h" // CSVToVector
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <iostream>

#include <zmq.h>

namespace adios2
{
namespace core
{
namespace engine
{

WdmReader::WdmReader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("WdmReader", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian())
{
    m_DataTransport = std::make_shared<transportman::StagingMan>(
        mpiComm, Mode::Read, m_Timeout, 1e9);
    m_MetadataTransport = std::make_shared<transportman::StagingMan>(
        mpiComm, Mode::Read, m_Timeout, 1e6);
    m_EndMessage = " in call to IO Open WdmReader " + m_Name + "\n";
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    Init();
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Open(" << m_Name
                  << ") in constructor." << std::endl;
    }
}

WdmReader::~WdmReader()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " deconstructor on "
                  << m_Name << "\n";
    }
}

StepStatus WdmReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{

    Log(5, "WdmReader::BeginStep() start. Last step " +
               std::to_string(m_CurrentStep),
        true, true);

    ++m_CurrentStep;

    auto reply = std::make_shared<std::vector<char>>();
    if (m_MpiRank == 0)
    {
        std::vector<char> request(1, 'M');
        reply = m_MetadataTransport->Request(
            request, m_FullAddresses[rand() % m_FullAddresses.size()]);
    }

    helper::BroadcastVector(*reply, m_MPIComm);

    if (reply->empty())
    {
        Log(1,
            "WdmReader::BeginStep() lost connection to writer. End of stream.",
            true, true);
        return StepStatus::EndOfStream;
    }

    m_DataManSerializer.PutAggregatedMetadata(m_MPIComm, reply);
    m_DataManSerializer.GetAttributes(m_IO);
    m_MetaDataMap = m_DataManSerializer.GetMetaData();

    size_t maxStep = std::numeric_limits<size_t>::min();
    size_t minStep = std::numeric_limits<size_t>::max();

    for (auto &i : m_MetaDataMap)
    {
        if (i.first > maxStep)
        {
            maxStep = i.first;
        }
        if (i.first < minStep)
        {
            minStep = i.first;
        }
    }

    if (stepMode == StepMode::NextAvailable)
    {
        m_CurrentStep = minStep;
    }
    else if (stepMode == StepMode::LatestAvailable)
    {
        m_CurrentStep = maxStep;
    }
    else
    {
        throw(std::invalid_argument(
            "[WdmReader::BeginStep] Step mode is not supported!"));
    }

    std::shared_ptr<std::vector<format::DataManSerializer::DataManVar>> vars =
        nullptr;
    auto currentStepIt = m_MetaDataMap.find(m_CurrentStep);
    if (currentStepIt != m_MetaDataMap.end())
    {
        vars = currentStepIt->second;
    }

    if (vars != nullptr)
    {

        for (const auto &i : *vars)
        {
            if (i.step == m_CurrentStep)
            {
                if (i.type == "compound")
                {
                    throw("Compound type is not supported yet.");
                }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
    }
                ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
                else
                {
                    throw("Unknown type caught in "
                          "DataManReader::BeginStepSubscribe.");
                }
            }
        }
    }

    Log(5, "WdmReader::BeginStep() start. Last step " +
               std::to_string(m_CurrentStep),
        true, true);

    return StepStatus::OK;
}

void WdmReader::PerformGets()
{

    Log(5, "WdmReader::PerformGets() begin", true, true);

    auto requests = m_DataManSerializer.GetDeferredRequest();

    if (m_Verbosity >= 10)
    {
        Log(10, "WdmReader::PerformGets() processing deferred requests ", true,
            false);
        for (const auto &i : *requests)
        {
            std::cout << i.first << ": ";
            for (auto j : i.second)
            {
                std::cout << j;
            }
            std::cout << std::endl;
        }
    }

    for (const auto &i : *requests)
    {
        auto reply = m_DataTransport->Request(i.second, i.first);
        if (reply->empty())
        {
            Log(1, "Lost connection to writer. Data for the final step is "
                   "corrupted!",
                true, true);
            return;
        }
        if (reply->size() <= 16)
        {
            std::string msg = "Step " + std::to_string(m_CurrentStep) +
                              " received empty data package from writer " +
                              i.first +
                              ". This may be caused by a network failure.";
            if (m_Tolerance)
            {
                Log(1, msg, true, true);
            }
            else
            {
                throw(std::runtime_error(msg));
            }
        }
        else
        {
            Log(6, "WdmReader::PerformGets() put reply of size " +
                       std::to_string(reply->size()) + " into serializer",
                true, true);
            m_DataManSerializer.PutPack(reply);
        }
    }

    auto varsVec = m_MetaDataMap.find(m_CurrentStep);
    if (varsVec == m_MetaDataMap.end())
    {
        return;
    }
    if (varsVec->second == nullptr)
    {
        return;
    }

    for (const auto &req : m_DeferredRequests)
    {
        if (req.type == "compound")
        {
            throw("Compound type is not supported yet.");
        }
#define declare_type(T)                                                        \
    else if (req.type == helper::GetType<T>())                                 \
    {                                                                          \
        m_DataManSerializer.GetVar(reinterpret_cast<T *>(req.data),            \
                                   req.variable, req.start, req.count,         \
                                   req.step);                                  \
        Log(6, "WdmReader::PerformGets() get variable " + req.variable +       \
                   " from serializer",                                         \
            true, true);                                                       \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    m_DeferredRequests.clear();

    Log(5, "WdmReader::PerformGets() end", true, true);
}

size_t WdmReader::CurrentStep() const { return m_CurrentStep; }

void WdmReader::EndStep()
{
    Log(5, "WdmReader::EndStep() start. Step " + std::to_string(m_CurrentStep),
        true, true);

    PerformGets();
    m_DataManSerializer.Erase(CurrentStep());

    Log(5, "WdmReader::EndStep() end. Step " + std::to_string(m_CurrentStep),
        true, true);
}

// PRIVATE

#define declare_type(T)                                                        \
    void WdmReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void WdmReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void WdmReader::Init()
{
    srand(time(NULL));
    InitParameters();
    Handshake();
}

void WdmReader::InitParameters()
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

void WdmReader::InitTransports() {}

void WdmReader::Handshake()
{
    transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
    while (true)
    {
        try
        {
            ipstream.Open(".StagingHandshake", Mode::Read);
            break;
        }
        catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    transport::FileFStream lockstream(m_MPIComm, m_DebugMode);
    while (true)
    {
        try
        {
            lockstream.Open(".StagingHandshakeLock", Mode::Read);
            lockstream.Close();
        }
        catch (...)
        {
            break;
        }
    }

    auto size = ipstream.GetSize();
    std::vector<char> address(size);
    ipstream.Read(address.data(), size);
    ipstream.Close();
    nlohmann::json j = nlohmann::json::parse(address);
    m_FullAddresses = j.get<std::vector<std::string>>();
}

void WdmReader::DoClose(const int transportIndex)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Close(" << m_Name
                  << ")\n";
    }
}

void WdmReader::Log(const int level, const std::string &message, const bool mpi,
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
