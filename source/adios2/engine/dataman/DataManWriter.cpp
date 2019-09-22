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
: DataManCommon("DataManWriter", io, name, openMode, std::move(comm))
{
    if (m_IPAddress.empty())
    {
        throw(std::invalid_argument(
            "IP address not specified in wide area staging"));
    }
    m_Port += m_MpiRank;
    m_ControlAddress = "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
    m_DataAddress =
        "tcp://" + m_IPAddress + ":" + std::to_string(m_Port + m_MpiSize);

    std::vector<std::string> daVec;
    std::vector<std::string> caVec;

    if (m_MpiSize == 1 or m_OneToOneMode)
    {
        daVec.push_back(m_DataAddress);
        caVec.push_back(m_ControlAddress);
    }
    else
    {
        std::vector<char> allDaVec(32 * m_MpiSize, '\0');
        std::vector<char> allCaVec(32 * m_MpiSize, '\0');

        m_Comm.Allgather(m_DataAddress.data(), m_DataAddress.size(),
                         allDaVec.data(), 32);
        m_Comm.Allgather(m_ControlAddress.data(), m_ControlAddress.size(),
                         allCaVec.data(), 32);

        for (int i = 0; i < m_MpiSize; ++i)
        {
            daVec.push_back(std::string(allDaVec.begin() + i * 32,
                                        allDaVec.begin() + (i + 1) * 32));
            caVec.push_back(std::string(allCaVec.begin() + i * 32,
                                        allCaVec.begin() + (i + 1) * 32));
        }
    }

    nlohmann::json addJson;
    addJson["DataAddresses"] = daVec;
    addJson["ControlAddresses"] = caVec;
    m_AllAddresses = addJson.dump() + '\0';

    m_ReplyThread =
        std::thread(&DataManWriter::ReplyThread, this, m_ControlAddress);

    m_DataPublisher.OpenPublisher(m_DataAddress, m_Timeout);

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::DataManWriter() Rank " << m_MpiRank
                  << std::endl;
    }
}

DataManWriter::~DataManWriter()
{
    if (not m_IsClosed)
    {
        DoClose();
    }
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::~DataManWriter() Rank " << m_MpiRank
                  << ", Fianl Step " << m_CurrentStep << std::endl;
    }
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    ++m_CurrentStep;
    m_FastSerializer.NewWriterBuffer(m_SerializerBufferSize);

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::BeginStep() Rank " << m_MpiRank
                  << ", Step " << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

void DataManWriter::PerformPuts() {}

void DataManWriter::EndStep()
{
    if (m_CurrentStep == 0)
    {
        m_FastSerializer.PutAttributes(m_IO);
    }

    m_FastSerializer.AttachAttributes();
    const auto buf = m_FastSerializer.GetLocalPack();
    m_SerializerBufferSize = buf->size();
    m_DataPublisher.PushBufferQueue(buf);

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::EndStep() Rank " << m_MpiRank << ", Step "
                  << m_CurrentStep << std::endl;
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
    m_DataPublisher.PushBufferQueue(cvp);

    m_ThreadActive = false;
    if (m_ReplyThread.joinable())
    {
        m_ReplyThread.join();
    }
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::DoClose() Rank " << m_MpiRank << ", Step "
                  << m_CurrentStep << std::endl;
    }
}

void DataManWriter::ReplyThread(const std::string &address)
{
    adios2::zmq::ZmqReqRep replier;
    replier.OpenReplier(address, m_Timeout, 8192);
    while (m_ThreadActive)
    {
        auto request = replier.ReceiveRequest();
        if (request && request->size() > 0)
        {
            std::string r(request->begin(), request->end());
            if (r == "Address")
            {
                replier.SendReply(m_AllAddresses.data(), m_AllAddresses.size());
            }
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
