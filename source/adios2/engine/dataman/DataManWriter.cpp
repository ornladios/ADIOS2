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
  m_Serializer(m_Comm, helper::IsRowMajor(io.m_HostLanguage))
{

    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();

    helper::GetParameter(m_IO.m_Parameters, "IPAddress", m_IPAddress);
    helper::GetParameter(m_IO.m_Parameters, "Port", m_Port);
    helper::GetParameter(m_IO.m_Parameters, "Timeout", m_Timeout);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "RendezvousReaderCount",
                         m_RendezvousReaderCount);
    helper::GetParameter(m_IO.m_Parameters, "DoubleBuffer", m_DoubleBuffer);
    helper::GetParameter(m_IO.m_Parameters, "TransportMode", m_TransportMode);
    helper::GetParameter(m_IO.m_Parameters, "Monitor", m_MonitorActive);

    if (m_IPAddress.empty())
    {
        throw(std::invalid_argument("IP address not specified"));
    }
    m_Port += m_MpiRank;
    m_ReplierAddress = "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
    m_PublisherAddress =
        "tcp://" + m_IPAddress + ":" + std::to_string(m_Port + m_MpiSize);

    std::vector<std::string> pubVec;
    std::vector<std::string> repVec;

    if (m_MpiSize == 1)
    {
        pubVec.push_back(m_PublisherAddress);
        repVec.push_back(m_ReplierAddress);
    }
    else
    {
        std::vector<char> allPubVec(32 * m_MpiSize, '\0');
        std::vector<char> allRepVec(32 * m_MpiSize, '\0');

        m_Comm.Allgather(m_PublisherAddress.data(), m_PublisherAddress.size(),
                         allPubVec.data(), 32);
        m_Comm.Allgather(m_ReplierAddress.data(), m_ReplierAddress.size(),
                         allRepVec.data(), 32);

        for (int i = 0; i < m_MpiSize; ++i)
        {
            pubVec.push_back(std::string(allPubVec.begin() + i * 32,
                                         allPubVec.begin() + (i + 1) * 32));
            repVec.push_back(std::string(allRepVec.begin() + i * 32,
                                         allRepVec.begin() + (i + 1) * 32));
        }
    }

    nlohmann::json addJson;
    addJson["PublisherAddresses"] = pubVec;
    addJson["ReplierAddresses"] = repVec;
    m_AllAddresses = addJson.dump() + '\0';

    if (m_TransportMode == "fast")
    {
        m_Publisher.OpenPublisher(m_PublisherAddress);
    }

    m_Replier.OpenReplier(m_ReplierAddress, m_Timeout, 64);

    if (m_RendezvousReaderCount == 0 || m_TransportMode == "reliable")
    {
        m_ReplyThread = std::thread(&DataManWriter::ReplyThread, this);
    }
    else
    {
        ReplyThread();
        m_Comm.Barrier();
    }

    if (m_DoubleBuffer && m_TransportMode == "fast")
    {
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
    m_Serializer.NewWriterBuffer(m_SerializerBufferSize);

    if (m_MonitorActive)
    {
        m_Monitor.BeginStep(m_CurrentStep);
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

    m_Serializer.AttachAttributesToLocalPack();
    const auto buffer = m_Serializer.GetLocalPack();
    if (buffer->size() > m_SerializerBufferSize)
    {
        m_SerializerBufferSize = buffer->size();
    }

    if (m_MonitorActive)
    {
        m_Monitor.BeginTransport(m_CurrentStep);
    }

    if (m_DoubleBuffer || m_TransportMode == "reliable")
    {
        PushBufferQueue(buffer);
    }
    else
    {
        m_Publisher.Send(buffer);
        if (m_MonitorActive)
        {
            m_Monitor.EndTransport();
        }
    }

    if (m_MonitorActive)
    {
        m_Monitor.EndStep(m_CurrentStep);
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
    nlohmann::json endSignal;
    endSignal["FinalStep"] = m_CurrentStep;
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

    m_ReplyThreadActive = false;
    m_PublishThreadActive = false;

    if (m_ReplyThread.joinable())
    {
        m_ReplyThread.join();
    }

    if (m_PublishThread.joinable())
    {
        m_PublishThread.join();
    }

    m_IsClosed = true;
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
            if (m_MonitorActive)
            {
                m_Monitor.EndTransport();
            }
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
            if (r == "Address")
            {
                m_Replier.SendReply(m_AllAddresses.data(),
                                    m_AllAddresses.size());
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
                    auto buffer = PopBufferQueue();
                }
                if (buffer != nullptr && buffer->size() > 0)
                {
                    m_Replier.SendReply(buffer);
                    if (m_MonitorActive)
                    {
                        m_Monitor.EndTransport();
                    }
                    if (buffer->size() < 64)
                    {
                        try
                        {
                            auto jmsg = nlohmann::json::parse(buffer->data());
                            auto finalStep = jmsg["FinalStep"].get<size_t>();
                            if (finalStep == m_CurrentStep)
                            {
                                m_ReplyThreadActive = false;
                            }
                        }
                        catch (...)
                        {
                        }
                    }
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
