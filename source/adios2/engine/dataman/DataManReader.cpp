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
#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace core
{
namespace engine
{

DataManReader::DataManReader(IO &io, const std::string &name,
                             const Mode openMode, helper::Comm comm)
: Engine("DataManReader", io, name, openMode, std::move(comm)),
  m_Serializer(m_Comm, helper::IsRowMajor(io.m_HostLanguage)),
  m_RequesterThreadActive(true), m_SubscriberThreadActive(true),
  m_FinalStep(std::numeric_limits<size_t>::max())
{
    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();
    helper::GetParameter(m_IO.m_Parameters, "IPAddress", m_IPAddress);
    helper::GetParameter(m_IO.m_Parameters, "Port", m_Port);
    helper::GetParameter(m_IO.m_Parameters, "Timeout", m_Timeout);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "DoubleBuffer", m_DoubleBuffer);
    helper::GetParameter(m_IO.m_Parameters, "TransportMode", m_TransportMode);
    helper::GetParameter(m_IO.m_Parameters, "Monitor", m_MonitorActive);

    m_Requesters.emplace_back();
    m_Requesters[0].OpenRequester(m_Timeout, m_ReceiverBufferSize);

    if (m_IPAddress.empty())
    {
        throw(std::invalid_argument(
            "IP address not specified in wide area staging"));
    }
    std::string address = "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
    std::string request = "Address";

    auto reply =
        m_Requesters[0].Request(request.data(), request.size(), address);

    auto start_time = std::chrono::system_clock::now();
    while (reply == nullptr or reply->empty())
    {
        reply =
            m_Requesters[0].Request(request.data(), request.size(), address);
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
    m_PublisherAddresses =
        addJson["PublisherAddresses"].get<std::vector<std::string>>();
    m_ReplierAddresses =
        addJson["ReplierAddresses"].get<std::vector<std::string>>();

    if (m_TransportMode == "fast")
    {
        for (const auto &address : m_PublisherAddresses)
        {
            m_Subscribers.emplace_back();
            m_Subscribers.back().OpenSubscriber(address, m_ReceiverBufferSize);
            m_SubscriberThreads.emplace_back(
                std::thread(&DataManReader::SubscribeThread, this,
                            std::ref(m_Subscribers.back())));
        }
    }
    else if (m_TransportMode == "reliable")
    {
        m_Requesters.clear();
        for (const auto &address : m_ReplierAddresses)
        {
            m_Requesters.emplace_back();
            m_Requesters.back().OpenRequester(address, m_Timeout,
                                              m_ReceiverBufferSize);
        }
    }
    else
    {
        throw(std::invalid_argument("unknown transport mode"));
    }

    for (auto &requester : m_Requesters)
    {
        requester.Request("Ready", 5, address);
    }

    if (m_TransportMode == "reliable")
    {
        for (const auto &address : m_ReplierAddresses)
        {
            m_RequesterThreads.emplace_back(
                std::thread(&DataManReader::RequestThread, this,
                            std::ref(m_Requesters.back())));
        }
    }
}

DataManReader::~DataManReader()
{
    if (not m_IsClosed)
    {
        DoClose();
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::~DataManReader() Rank " << m_MpiRank
                  << ", Step " << m_CurrentStep << std::endl;
    }
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::BeginStep() begin, Rank " << m_MpiRank
                  << ", Step " << m_CurrentStep << std::endl;
    }

    float timeout = timeoutSeconds;

    if (timeout <= 0)
    {
        timeout = m_Timeout;
    }

    if (m_InitFailed)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManReader::BeginStep(), Rank " << m_MpiRank
                      << " returned EndOfStream due "
                         "to initialization failure"
                      << std::endl;
        }
        return StepStatus::EndOfStream;
    }

    if (m_CurrentStep >= m_FinalStep and m_CurrentStep >= 0)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManReader::BeginStep() Rank " << m_MpiRank
                      << " returned EndOfStream, "
                         "final step is "
                      << m_FinalStep << std::endl;
        }
        return StepStatus::EndOfStream;
    }

    m_CurrentStepMetadata = m_Serializer.GetEarliestLatestStep(
        m_CurrentStep, m_PublisherAddresses.size(), timeout, true);

    if (m_CurrentStepMetadata == nullptr)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManReader::BeginStep() Rank " << m_MpiRank
                      << "returned EndOfStream due "
                         "to timeout"
                      << std::endl;
        }
        return StepStatus::EndOfStream;
    }

    m_Serializer.GetAttributes(m_IO);

    for (const auto &i : *m_CurrentStepMetadata)
    {
        if (i.step == m_CurrentStep)
        {
            if (i.type == DataType::None)
            {
                throw("unknown data type");
            }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetDataType<T>())                               \
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
        std::cout << "DataManReader::BeginStep() end, Rank " << m_MpiRank
                  << ", Step " << m_CurrentStep << std::endl;
    }

    if (m_MonitorActive)
    {
        m_Monitor.BeginStep(m_CurrentStep);
    }

    return StepStatus::OK;
}

size_t DataManReader::CurrentStep() const { return m_CurrentStep; }

void DataManReader::PerformGets() {}

void DataManReader::EndStep()
{
    m_Serializer.Erase(m_CurrentStep, true);
    m_CurrentStepMetadata = nullptr;

    if (m_MonitorActive)
    {
        m_Monitor.EndStep(m_CurrentStep);
    }
}

void DataManReader::Flush(const int transportIndex) {}

// PRIVATE

void DataManReader::RequestThread(zmq::ZmqReqRep &requester)
{
    while (m_RequesterThreadActive)
    {
        auto buffer = requester.Request("Step", 4);
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
            m_Serializer.PutPack(buffer, m_DoubleBuffer);
        }
    }
}

void DataManReader::SubscribeThread(zmq::ZmqPubSub &subscriber)
{
    while (m_SubscriberThreadActive)
    {
        auto buffer = subscriber.Receive();
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
            m_Serializer.PutPack(buffer, m_DoubleBuffer);
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
    m_SubscriberThreadActive = false;
    m_RequesterThreadActive = false;
    for (auto &t : m_SubscriberThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    for (auto &t : m_RequesterThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    m_IsClosed = true;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
