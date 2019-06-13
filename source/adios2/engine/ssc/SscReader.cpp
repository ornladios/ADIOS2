/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscReader.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "SscReader.h"
#include "SscReader.tcc"

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

SscReader::SscReader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("SscReader", io, name, mode, mpiComm),
  m_DataManSerializer(helper::IsRowMajor(io.m_HostLanguage), true,
                      helper::IsLittleEndian(), mpiComm),
  m_RepliedMetadata(std::make_shared<std::vector<char>>())
{
    TAU_SCOPED_TIMER_FUNC();
    m_DataTransport = std::make_shared<transportman::StagingMan>(
        mpiComm, Mode::Read, m_Timeout, 1e9);
    m_MetadataTransport = std::make_shared<transportman::StagingMan>(
        mpiComm, Mode::Read, m_Timeout, 1e8);
    m_EndMessage = " in call to IO Open SscReader " + m_Name + "\n";
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    Init();
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Open(" << m_Name
                  << ") in constructor." << std::endl;
    }
}

SscReader::~SscReader()
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " destructor on "
                  << m_Name << "\n";
    }
    m_MetadataTransport->CloseTransport();
    m_DataTransport->CloseTransport();
    m_DataTransport = nullptr;
    m_MetadataTransport = nullptr;
}

StepStatus SscReader::BeginStepIterator(StepMode stepMode,
                                        format::DmvVecPtr &vars)
{
    TAU_SCOPED_TIMER_FUNC();

    RequestMetadata();
    m_MetaDataMap = m_DataManSerializer.GetMetaData();

    if (m_MetaDataMap.empty())
    {
        Log(5,
            "SscReader::BeginStepIterator() returned NotReady because the "
            "metadata map is empty",
            true, true);
        return StepStatus::NotReady;
    }

    size_t maxStep = std::numeric_limits<size_t>::min();
    for (auto &i : m_MetaDataMap)
    {
        if (i.first > maxStep)
        {
            maxStep = i.first;
        }
    }
    if (m_CurrentStep > maxStep)
    {
        Log(50,
            "SscReader::BeginStepIterator() returned NotReady because no new "
            "step is received yet",
            true, true);
        return StepStatus::NotReady;
    }
    else
    {
        m_CurrentStep = maxStep;
    }

    auto currentStepIt = m_MetaDataMap.find(m_CurrentStep);
    if (currentStepIt == m_MetaDataMap.end())
    {
        Log(5,
            "SscReader::BeginStepIterator() returned NotReady because the "
            "current step is not existed in metadata map",
            true, true);
        return StepStatus::NotReady;
    }
    else
    {
        vars = currentStepIt->second;
    }

    if (vars == nullptr)
    {
        Log(5,
            "SscReader::BeginStepIterator() returned NotReady because vars == "
            "nullptr",
            true, true);
        return StepStatus::NotReady;
    }

    return StepStatus::OK;
}

StepStatus SscReader::BeginStep(const StepMode stepMode,
                                const float timeoutSeconds)
{

    TAU_SCOPED_TIMER_FUNC();
    Log(5,
        "SscReader::BeginStep() start. Last step " +
            std::to_string(m_CurrentStep),
        true, true);

    ++m_CurrentStep;

    format::DmvVecPtr vars = nullptr;

    auto startTime = std::chrono::system_clock::now();

    while (vars == nullptr)
    {
        auto stepStatus = BeginStepIterator(stepMode, vars);
        if (stepStatus == StepStatus::OK)
        {
            break;
        }
        else if (stepStatus == StepStatus::EndOfStream)
        {
            return StepStatus::EndOfStream;
        }
        if (timeoutSeconds >= 0)
        {
            auto nowTime = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                nowTime - startTime);
            if (duration.count() > timeoutSeconds)
            {
                Log(5,
                    "SscReader::BeginStep() returned EndOfStream because of "
                    "timeout.",
                    true, true);
                --m_CurrentStep;
                return StepStatus::NotReady;
            }
        }
    }

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
        CheckIOVariable<T>(i.name, i.shape);                                   \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else
            {
                throw("Unknown type caught in "
                      "DataManReader::BeginStepSubscribe.");
            }
        }
    }

    Log(5,
        "SscReader::BeginStep() start. Last step " +
            std::to_string(m_CurrentStep),
        true, true);

    return StepStatus::OK;
}

void SscReader::PerformGets()
{

    TAU_SCOPED_TIMER_FUNC();
    Log(5, "SscReader::PerformGets() begin", true, true);

    auto requests = m_DataManSerializer.GetDeferredRequest();

    if (m_Verbosity >= 10)
    {
        Log(10, "SscReader::PerformGets() processing deferred requests ", true,
            true);
        for (const auto &i : *requests)
        {
            std::cout << i.first << ": ";
            for (auto j : *i.second)
            {
                std::cout << j;
            }
            std::cout << std::endl;
        }
    }

    for (const auto &i : *requests)
    {
        auto reply = m_DataTransport->Request(*i.second, i.first);
        if (reply->empty())
        {
            Log(1,
                "Lost connection to writer. Data for the final step is "
                "corrupted!",
                true, true);
            m_ConnectionLost = true;
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
            Log(6,
                "SscReader::PerformGets() put reply of size " +
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
        Log(6,                                                                 \
            "SscReader::PerformGets() get variable " + req.variable +          \
                " from serializer",                                            \
            true, true);                                                       \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    m_DeferredRequests.clear();

    Log(5, "SscReader::PerformGets() end", true, true);
}

size_t SscReader::CurrentStep() const { return m_CurrentStep; }

void SscReader::EndStep()
{
    TAU_SCOPED_TIMER_FUNC();
    Log(5, "SscReader::EndStep() start. Step " + std::to_string(m_CurrentStep),
        true, true);

    PerformGets();
    m_DataManSerializer.Erase(CurrentStep(), true);

    Log(5, "SscReader::EndStep() end. Step " + std::to_string(m_CurrentStep),
        true, true);
}

// PRIVATE

#define declare_type(T)                                                        \
    void SscReader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void SscReader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    SscReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        return AllStepsBlocksInfoCommon(variable);                             \
    }                                                                          \
    std::vector<typename Variable<T>::Info> SscReader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return BlocksInfoCommon(variable, step);                               \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SscReader::Init()
{
    TAU_SCOPED_TIMER_FUNC();
    srand(time(NULL));
    InitParameters();
    helper::HandshakeReader(m_MPIComm, m_AppID, m_FullAddresses, m_Name, "ssc");

    format::VecPtr reply = std::make_shared<std::vector<char>>();
    if (m_MpiRank == 0)
    {
        std::vector<char> request(2 * sizeof(int64_t));
        reinterpret_cast<int64_t *>(request.data())[0] = m_AppID;
        reinterpret_cast<int64_t *>(request.data())[1] = -3;
        std::string address = m_FullAddresses[rand() % m_FullAddresses.size()];
        while (reply->size() < 6)
        {
            reply = m_MetadataTransport->Request(request, address);
        }
    }
    m_DataManSerializer.PutAggregatedMetadata(reply, m_MPIComm);
    m_DataManSerializer.GetAttributes(m_IO);

    if (m_Verbosity >= 5)
    {
        for (const auto &i : m_FullAddresses)
        {
            std::cout << i << std::endl;
        }
    }
}

void SscReader::InitParameters()
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
    }
}

void SscReader::InitTransports() {}

void SscReader::RequestMetadata(const int64_t step)
{
    TAU_SCOPED_TIMER_FUNC();
    format::VecPtr reply = std::make_shared<std::vector<char>>();
    if (m_MpiRank == 0)
    {
        std::vector<char> request(2 * sizeof(int64_t));
        reinterpret_cast<int64_t *>(request.data())[0] = m_AppID;
        reinterpret_cast<int64_t *>(request.data())[1] = step;
        std::string address = m_FullAddresses[rand() % m_FullAddresses.size()];
        reply = m_MetadataTransport->Request(request, address);
    }
    m_DataManSerializer.PutAggregatedMetadata(reply, m_MPIComm);
}

void SscReader::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER_FUNC();
    if (m_Verbosity >= 5)
    {
        std::cout << "Staging Reader " << m_MpiRank << " Close(" << m_Name
                  << ")\n";
    }
}

void SscReader::Log(const int level, const std::string &message, const bool mpi,
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
