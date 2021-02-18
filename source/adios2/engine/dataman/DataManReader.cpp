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
    helper::GetParameter(m_IO.m_Parameters, "Threading", m_Threading);
    helper::GetParameter(m_IO.m_Parameters, "Monitor", m_MonitorActive);
    helper::GetParameter(m_IO.m_Parameters, "MaxStepBufferSize",
                         m_ReceiverBufferSize);

    if (m_IPAddress.empty())
    {
        throw(std::invalid_argument(
            "IP address not specified in wide area staging"));
    }

    std::string requesterAddress =
        "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
    std::string subscriberAddress =
        "tcp://" + m_IPAddress + ":" + std::to_string(m_Port + 1);
    m_Requester.OpenRequester(requesterAddress, m_Timeout,
                              m_ReceiverBufferSize);

    auto timeBeforeRequest = std::chrono::system_clock::now();
    auto reply = m_Requester.Request("Handshake", 9);
    auto timeAfterRequest = std::chrono::system_clock::now();
    auto roundLatency = std::chrono::duration_cast<std::chrono::milliseconds>(
                            timeAfterRequest - timeBeforeRequest)
                            .count();

    auto startTime = std::chrono::system_clock::now();

    while (reply == nullptr or reply->empty())
    {
        reply = m_Requester.Request("Handshake", 9);
        auto nowTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            nowTime - startTime);
        roundLatency = duration.count() * 1000;
        if (duration.count() > m_Timeout)
        {
            m_InitFailed = true;
            return;
        }
    }

    nlohmann::json message = nlohmann::json::parse(reply->data());
    m_TransportMode = message["Transport"];

    if (m_MonitorActive)
    {
        m_Monitor.SetClockError(roundLatency, message["TimeStamp"]);
        m_Monitor.AddTransport(m_TransportMode);
        if (m_Threading)
        {
            m_Monitor.SetReaderThreading();
        }
        bool writerThreading = message["Threading"];
        if (writerThreading)
        {
            m_Monitor.SetWriterThreading();
        }
        m_Monitor.SetRequiredAccuracy(
            stof(message["FloatAccuracy"].get<std::string>()));
    }

    if (m_TransportMode == "fast")
    {
        m_Subscriber.OpenSubscriber(subscriberAddress, m_ReceiverBufferSize);
        m_SubscriberThread = std::thread(&DataManReader::SubscribeThread, this);
    }
    else if (m_TransportMode == "reliable")
    {
    }
    else
    {
        throw(std::invalid_argument("unknown transport mode"));
    }

    m_Requester.Request("Ready", 5);

    if (m_TransportMode == "reliable")
    {
        m_RequesterThread = std::thread(&DataManReader::RequestThread, this);
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

    m_CurrentStepMetadata =
        m_Serializer.GetEarliestLatestStep(m_CurrentStep, 1, timeout, false);

    if (m_CurrentStepMetadata == nullptr)
    {
        if (m_Verbosity >= 5)
        {
            std::cout << "DataManReader::BeginStep() Rank " << m_MpiRank
                      << " returned EndOfStream due "
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
        auto comMap = m_Serializer.GetOperatorMap();
        for (const auto &i : comMap)
        {
            std::string method, accuracy;
            auto it = i.second.find("accuracy");
            if (it != i.second.end())
            {
                accuracy = it->second;
            }
            it = i.second.find("method");
            if (it != i.second.end())
            {
                method = it->second;
            }
            m_Monitor.AddCompression(method, std::stof(accuracy));
        }
        m_Monitor.EndStep(m_CurrentStep);
    }
}

void DataManReader::Flush(const int transportIndex) {}

// PRIVATE

void DataManReader::RequestThread()
{
    while (m_RequesterThreadActive)
    {
        std::string request = "Step";
        auto buffer = m_Requester.Request(request.data(), request.size());
        if (buffer != nullptr && buffer->size() > 0)
        {
            if (buffer->size() < 64)
            {
                try
                {
                    auto jmsg = nlohmann::json::parse(buffer->data());
                    m_FinalStep = jmsg["FinalStep"].get<size_t>();
                    return;
                }
                catch (...)
                {
                }
            }
            m_Serializer.PutPack(buffer, m_Threading);
            if (m_MonitorActive)
            {
                size_t combiningSteps = m_Serializer.GetCombiningSteps();
                if (combiningSteps < 20)
                {
                    m_Monitor.SetAverageSteps(40);
                }
                else
                {
                    m_Monitor.SetAverageSteps(combiningSteps * 2);
                }
                auto timeStamps = m_Serializer.GetTimeStamps();
                for (const auto &timeStamp : timeStamps)
                {
                    m_Monitor.AddLatencyMilliseconds(timeStamp);
                }
            }
        }
    }
}

void DataManReader::SubscribeThread()
{
    while (m_SubscriberThreadActive)
    {
        auto buffer = m_Subscriber.Receive();
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
            m_Serializer.PutPack(buffer, m_Threading);
            if (m_MonitorActive)
            {
                size_t combiningSteps = m_Serializer.GetCombiningSteps();
                if (combiningSteps < 20)
                {
                    m_Monitor.SetAverageSteps(40);
                }
                else
                {
                    m_Monitor.SetAverageSteps(combiningSteps * 2);
                }
                auto timeStamps = m_Serializer.GetTimeStamps();
                for (const auto &timeStamp : timeStamps)
                {
                    m_Monitor.AddLatencyMilliseconds(timeStamp);
                }
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
    m_SubscriberThreadActive = false;
    m_RequesterThreadActive = false;
    if (m_SubscriberThread.joinable())
    {
        m_SubscriberThread.join();
    }
    if (m_RequesterThread.joinable())
    {
        m_RequesterThread.join();
    }
    m_IsClosed = true;
    if (m_MonitorActive)
    {
        m_Monitor.OutputJson(m_Name);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
