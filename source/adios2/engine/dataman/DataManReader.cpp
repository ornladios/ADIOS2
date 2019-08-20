/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: Jason Wang
 *              William F Godoy
 */

#include "DataManReader.h"
#include "DataManReader.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{
namespace core
{
namespace engine
{

DataManReader::DataManReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: DataManCommon("DataManReader", io, name, mode, mpiComm)
{
    GetParameter(m_IO.m_Parameters, "AlwaysProvideLatestTimestep",
                 m_ProvideLatest);

    m_ZmqRequester.OpenRequester(m_Timeout, m_ReceiverBufferSize);

    if (m_StagingMode == "wide")
    {
        if (m_IPAddress.empty())
        {
            throw(std::invalid_argument(
                "IP address not specified in wide area staging"));
        }
        std::string address =
            "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
        std::string request = "Address";
        auto reply =
            m_ZmqRequester.Request(request.data(), request.size(), address);

        auto start_time = std::chrono::system_clock::now();
        while (reply == nullptr or reply->empty())
        {
            reply =
                m_ZmqRequester.Request(request.data(), request.size(), address);
            auto now_time = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now_time - start_time);
            if (duration.count() > m_Timeout)
            {
                m_InitFailed = true;
                return;
            }
        }
        auto addJson = nlohmann::json::parse(*reply);
        m_DataAddresses =
            addJson["DataAddresses"].get<std::vector<std::string>>();
        m_ControlAddresses =
            addJson["ControlAddresses"].get<std::vector<std::string>>();
    }
    else if (m_StagingMode == "local")
    {
        // TODO: Add filesystem based handshake
    }

    for (const auto &address : m_DataAddresses)
    {
        auto dataZmq = std::make_shared<adios2::zmq::ZmqPubSub>();
        dataZmq->OpenSubscriber(address, m_Timeout, m_ReceiverBufferSize);
        m_ZmqSubscriberVec.push_back(dataZmq);
    }

    m_SubscriberThread = std::thread(&DataManReader::SubscriberThread, this);
}

DataManReader::~DataManReader()
{
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::~DataManReader() Step " << m_CurrentStep
                  << std::endl;
    }
    if (not m_IsClosed)
    {
        DoClose();
    }
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::BeginStep() begin. Last step "
                  << m_CurrentStep << std::endl;
    }

    if (m_InitFailed or (m_CurrentStep == m_FinalStep && m_CurrentStep > 0))
    {
        return StepStatus::EndOfStream;
    }

    format::DmvVecPtr vars = nullptr;
    auto start_time = std::chrono::system_clock::now();

    while (vars == nullptr)
    {
        m_MetaDataMap = m_DataManSerializer.GetMetaData();

        if (m_ProvideLatest)
        {
            size_t maxStep = 0;
            for (const auto &i : m_MetaDataMap)
            {
                if (maxStep < i.first)
                {
                    maxStep = i.first;
                }
            }
            m_CurrentStep = maxStep;
        }
        else
        {
            size_t minStep = std::numeric_limits<size_t>::max();
            for (const auto &i : m_MetaDataMap)
            {
                if (minStep > i.first)
                {
                    minStep = i.first;
                }
            }
            m_CurrentStep = minStep;
        }

        auto currentStepIt = m_MetaDataMap.find(m_CurrentStep);
        if (currentStepIt == m_MetaDataMap.end())
        {
            auto now_time = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now_time - start_time);
            if (duration.count() > timeoutSeconds)
            {
                return StepStatus::NotReady;
            }
        }
        else
        {
            vars = currentStepIt->second;
        }
    }

    m_DataManSerializer.GetAttributes(m_IO);

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
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else
            {
                throw("Unknown type caught in "
                      "DataManReader::BeginStepSubscribe.");
            }
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::BeginStep() end. Current step "
                  << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t DataManReader::CurrentStep() const { return m_CurrentStep; }

void DataManReader::PerformGets() {}

void DataManReader::EndStep()
{
    m_DataManSerializer.Erase(m_CurrentStep);
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::EndStep() end. Current step "
                  << m_CurrentStep << std::endl;
    }
}

void DataManReader::Flush(const int transportIndex) {}

// PRIVATE

void DataManReader::SubscriberThread()
{
    while (m_ThreadActive)
    {
        for (auto &z : m_ZmqSubscriberVec)
        {
            auto buffer = z->PopBufferQueue();
            if (buffer != nullptr && buffer->size() > 0)
            {
                // check if is control signal
                if (buffer->size() < 64)
                {
                    try
                    {
                        nlohmann::json jmsg =
                            nlohmann::json::parse(buffer->data());
                        m_FinalStep = jmsg["FinalStep"].get<size_t>();
                        continue;
                    }
                    catch (std::exception)
                    {
                    }
                }
                m_DataManSerializer.PutPack(buffer);
            }
        }
    }
}

#define declare_type(T)                                                        \
    void DataManReader::DoGetSync(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void DataManReader::DoGetDeferred(Variable<T> &variable, T *data)          \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    DataManReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const     \
    {                                                                          \
        return AllStepsBlocksInfoCommon(variable);                             \
    }                                                                          \
    std::vector<typename Variable<T>::Info> DataManReader::DoBlocksInfo(       \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return BlocksInfoCommon(variable, step);                               \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void DataManReader::DoClose(const int transportIndex)
{
    m_ThreadActive = false;
    if (m_SubscriberThread.joinable())
    {
        m_SubscriberThread.join();
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
