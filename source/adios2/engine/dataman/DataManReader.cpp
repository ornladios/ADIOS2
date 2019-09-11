/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: Jason Wang
 */

#include "DataManReader.tcc"

namespace adios2
{
namespace core
{
namespace engine
{

DataManReader::DataManReader(IO &io, const std::string &name, const Mode mode,
                             helper::Comm comm)
: DataManCommon("DataManReader", io, name, mode, std::move(comm))
{
    GetParameter(m_IO.m_Parameters, "AlwaysProvideLatestTimestep",
                 m_ProvideLatest);

    if (m_MpiRank == m_MpiSize - 1)
    {
        m_StreamingRank = true;
    }
    else
    {
        m_StreamingRank = false;
    }

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
        m_TotalWriters = m_DataAddresses.size();
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

    if (m_Reliable)
    {
    }
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

    if (m_InitFailed)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManReader::BeginStep() returned EndOfStream due "
                         "to initialization failure"
                      << std::endl;
        }
        return StepStatus::EndOfStream;
    }

    if (m_CurrentStep >= m_FinalStep and m_CurrentStep >= 0)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << m_CurrentStep << " >= " << m_FinalStep << std::endl;
            std::cout << "DataManReader::BeginStep() returned EndOfStream, "
                         "final step is "
                      << m_FinalStep << std::endl;
        }
        return StepStatus::EndOfStream;
    }

    m_CurrentStepMetadata = m_FastSerializer.GetEarliestLatestStep(
        m_CurrentStep, m_TotalWriters, timeoutSeconds, m_ProvideLatest);

    if (m_CurrentStepMetadata == nullptr)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManReader::BeginStep() returned EndOfStream, "
                         "final step is "
                      << m_FinalStep << std::endl;
        }
        return StepStatus::EndOfStream;
    }

    m_FastSerializer.GetAttributes(m_IO);

    for (const auto &i : *m_CurrentStepMetadata)
    {
        if (i.step == m_CurrentStep)
        {
            if (i.type.empty())
            {
                throw("unknown data type");
            }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
    }
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else { throw("unknown data type"); }
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
    m_FastSerializer.Erase(m_CurrentStep);
    m_CurrentStepMetadata = nullptr;
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::EndStep() Current step " << m_CurrentStep
                  << std::endl;
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
                if (buffer->size() < 64)
                {
                    try
                    {
                        auto jmsg = nlohmann::json::parse(buffer->data());
                        m_FinalStep = jmsg["FinalStep"].get<size_t>();
                        continue;
                    }
                    catch (...)
                    {
                    }
                }
                m_FastSerializer.PutPack(buffer);
            }
        }
    }
}

void DataManReader::RequesterThread() {}

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
