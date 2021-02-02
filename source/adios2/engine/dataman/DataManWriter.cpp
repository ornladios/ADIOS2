/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManWriter.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: Jason Wang
 */

#include "DataManWriter.tcc"

namespace adios2
{
namespace core
{
namespace engine
{

DataManWriter::DataManWriter(IO &io, const std::string &name,
                             const Mode openMode, helper::Comm comm)
: Engine("DataManWriter", io, name, openMode, std::move(comm)),
  m_Serializer(m_Comm, helper::IsRowMajor(io.m_HostLanguage)), m_SentSteps(0),
  m_ReplyThreadActive(true), m_PublishThreadActive(true)
{

    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();

    if (m_MpiSize > 1)
    {
        std::cerr << "DataMan does not support N-to-M decomposition!"
                  << std::endl;
    }

    helper::GetParameter(m_IO.m_Parameters, "IPAddress", m_IPAddress);
    helper::GetParameter(m_IO.m_Parameters, "Port", m_Port);
    helper::GetParameter(m_IO.m_Parameters, "Timeout", m_Timeout);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "RendezvousReaderCount",
                         m_RendezvousReaderCount);
    helper::GetParameter(m_IO.m_Parameters, "DoubleBuffer", m_DoubleBuffer);
    helper::GetParameter(m_IO.m_Parameters, "TransportMode", m_TransportMode);
    helper::GetParameter(m_IO.m_Parameters, "Monitor", m_MonitorActive);
    helper::GetParameter(m_IO.m_Parameters, "CombiningSteps", m_CombiningSteps);

    if (m_IPAddress.empty())
    {
        throw(std::invalid_argument("IP address not specified"));
    }

    if (m_MonitorActive)
    {
        if (m_CombiningSteps < 20)
        {
            m_Monitor.SetAverageSteps(40);
        }
        else
        {
            m_Monitor.SetAverageSteps(m_CombiningSteps * 2);
        }
    }

    std::string replierAddress =
        "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
    std::string publisherAddress =
        "tcp://" + m_IPAddress + ":" + std::to_string(m_Port + 1);

    if (m_TransportMode == "fast")
    {
        m_Publisher.OpenPublisher(publisherAddress);
    }

    m_Replier.OpenReplier(replierAddress, m_Timeout, 64);

    if (m_RendezvousReaderCount == 0 || m_TransportMode == "reliable")
    {
        m_ReplyThreadActive = true;
        m_ReplyThread = std::thread(&DataManWriter::ReplyThread, this);
    }
    else
    {
        ReplyThread();
    }

    if (m_DoubleBuffer && m_TransportMode == "fast")
    {
        m_PublishThreadActive = true;
        m_PublishThread = std::thread(&DataManWriter::PublishThread, this);
    }
}

DataManWriter::~DataManWriter()
{
    if (not m_IsClosed)
    {
        DoClose();
    }
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    ++m_CurrentStep;
    if (m_CombinedSteps == 0)
    {
        m_Serializer.NewWriterBuffer(m_SerializerBufferSize);
    }

    if (m_MonitorActive)
    {
        m_Monitor.BeginStep(m_CurrentStep);
    }

    if (m_Verbosity >= 10)
    {
        std::cout << "DataManWriter::BeginStep " << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

void DataManWriter::PerformPuts() {}

void DataManWriter::EndStep()
{
    if (m_CurrentStep == 0)
    {
        m_Serializer.PutAttributes(m_IO);
    }

    m_Serializer.AttachTimeStamp(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());

    ++m_CombinedSteps;

    if (m_CombinedSteps >= m_CombiningSteps)
    {
        m_CombinedSteps = 0;
        m_Serializer.AttachAttributesToLocalPack();
        const auto buffer = m_Serializer.GetLocalPack();
        if (buffer->size() > m_SerializerBufferSize)
        {
            m_SerializerBufferSize = buffer->size();
        }

        if (m_DoubleBuffer || m_TransportMode == "reliable")
        {
            PushBufferQueue(buffer);
        }
        else
        {
            m_Publisher.Send(buffer);
        }
    }

    if (m_MonitorActive)
    {
        m_Monitor.EndStep(m_CurrentStep);
    }

    if (m_Verbosity >= 10)
    {
        std::cout << "DataManWriter::EndStep " << m_CurrentStep << std::endl;
    }
}

void DataManWriter::Flush(const int transportIndex) {}

// PRIVATE functions below

#define declare_type(T)                                                        \
    void DataManWriter::DoPutSync(Variable<T> &variable, const T *values)      \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void DataManWriter::DoPutDeferred(Variable<T> &variable, const T *values)  \
    {                                                                          \
        PutDeferredCommon(variable, values);                                   \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void DataManWriter::DoClose(const int transportIndex)
{
    if (m_CombinedSteps < m_CombiningSteps && m_CombinedSteps > 0)
    {
        m_Serializer.AttachAttributesToLocalPack();
        const auto buffer = m_Serializer.GetLocalPack();
        if (buffer->size() > m_SerializerBufferSize)
        {
            m_SerializerBufferSize = buffer->size();
        }

        if (m_DoubleBuffer || m_TransportMode == "reliable")
        {
            PushBufferQueue(buffer);
        }
        else
        {
            m_Publisher.Send(buffer);
        }
    }

    nlohmann::json endSignal;
    endSignal["FinalStep"] = static_cast<int64_t>(m_CurrentStep);
    std::string s = endSignal.dump() + '\0';
    auto cvp = std::make_shared<std::vector<char>>(s.size());
    std::memcpy(cvp->data(), s.c_str(), s.size());

    if (m_DoubleBuffer || m_TransportMode == "reliable")
    {
        PushBufferQueue(cvp);
    }
    else
    {
        m_Publisher.Send(cvp);
    }

    m_PublishThreadActive = false;

    if (m_ReplyThreadActive)
    {
        while (m_SentSteps < m_CurrentStep + 2)
        {
        }
        m_ReplyThreadActive = false;
    }

    if (m_ReplyThread.joinable())
    {
        m_ReplyThread.join();
    }

    if (m_PublishThread.joinable())
    {
        m_PublishThread.join();
    }

    m_IsClosed = true;

    if (m_Verbosity >= 10)
    {
        std::cout << "DataManWriter::DoClose " << m_CurrentStep << std::endl;
    }
}

void DataManWriter::PushBufferQueue(std::shared_ptr<std::vector<char>> buffer)
{
    std::lock_guard<std::mutex> l(m_BufferQueueMutex);
    m_BufferQueue.push(buffer);
}

std::shared_ptr<std::vector<char>> DataManWriter::PopBufferQueue()
{
    std::lock_guard<std::mutex> l(m_BufferQueueMutex);
    if (m_BufferQueue.empty())
    {
        return nullptr;
    }
    else
    {
        auto ret = m_BufferQueue.front();
        m_BufferQueue.pop();
        return ret;
    }
}

void DataManWriter::PublishThread()
{
    while (m_PublishThreadActive)
    {
        auto buffer = PopBufferQueue();
        if (buffer != nullptr && buffer->size() > 0)
        {
            m_Publisher.Send(buffer);
        }
    }
}

void DataManWriter::ReplyThread()
{
    int readerCount = 0;
    while (m_ReplyThreadActive)
    {
        auto request = m_Replier.ReceiveRequest();
        if (request != nullptr && request->size() > 0)
        {
            std::string r(request->begin(), request->end());
            if (r == "Handshake")
            {
                uint64_t timeStamp =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
                m_Replier.SendReply(&timeStamp, sizeof(timeStamp));
            }
            else if (r == "Ready")
            {
                m_Replier.SendReply("OK", 2);
                ++readerCount;
            }
            else if (r == "Step")
            {
                auto buffer = PopBufferQueue();
                while (buffer == nullptr)
                {
                    buffer = PopBufferQueue();
                }
                if (buffer->size() > 0)
                {
                    m_Replier.SendReply(buffer);
                    m_SentSteps = m_SentSteps + m_CombiningSteps;
                }
            }
        }
        if (m_RendezvousReaderCount == readerCount && m_TransportMode == "fast")
        {
            m_ReplyThreadActive = false;
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
