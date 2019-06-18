/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#include "DataManWriter.h"
#include "DataManWriter.tcc"

#include <iostream>

#include "adios2/common/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{
namespace core
{
namespace engine
{

DataManWriter::DataManWriter(IO &io, const std::string &name,
                             const Mode openMode, MPI_Comm mpiComm)
: DataManCommon("DataManWriter", io, name, openMode, mpiComm)
{
    if (m_StagingMode == "wide")
    {
        if (m_IPAddress.empty())
        {
            throw(std::invalid_argument(
                "IP address not specified in wide area staging"));
        }
        m_Port += m_MpiRank;
        m_ControlAddress =
            "tcp://" + m_IPAddress + ":" + std::to_string(m_Port);
        m_DataAddress =
            "tcp://" + m_IPAddress + ":" + std::to_string(m_Port + m_MpiSize);

        std::vector<char> allDaVec(32 * m_MpiSize, '\0');
        std::vector<char> allCaVec(32 * m_MpiSize, '\0');

        MPI_Allgather(m_DataAddress.data(), m_DataAddress.size(), MPI_CHAR,
                      allDaVec.data(), 32, MPI_CHAR, m_Comm);
        MPI_Allgather(m_ControlAddress.data(), m_ControlAddress.size(),
                      MPI_CHAR, allCaVec.data(), 32, MPI_CHAR, m_Comm);

        std::vector<std::string> daVec;
        std::vector<std::string> caVec;

        for (int i = 0; i < m_MpiSize; ++i)
        {
            daVec.push_back(std::string(allDaVec.begin() + i * 32,
                                        allDaVec.begin() + (i + 1) * 32));
            caVec.push_back(std::string(allCaVec.begin() + i * 32,
                                        allCaVec.begin() + (i + 1) * 32));
        }

        nlohmann::json addJson;
        addJson["DataAddresses"] = daVec;
        addJson["ControlAddresses"] = caVec;

        m_AllAddresses = addJson.dump() + '\0';
    }
    else if (m_StagingMode == "local")
    {
        // TODO: Add filesystem based handshake
    }

    m_DataPublisher.OpenPublisher(m_DataAddress, m_Timeout);

    m_ControlThread =
        std::thread(&DataManWriter::ControlThread, this, m_ControlAddress);
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
    m_DataManSerializer.NewWriterBuffer(m_SerializerBufferSize);

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
        m_DataManSerializer.PutAttributes(m_IO);
    }
    m_DataManSerializer.AttachAttributes();
    const auto buf = m_DataManSerializer.GetLocalPack();
    m_SerializerBufferSize = buf->size();
    m_DataPublisher.PushBufferQueue(buf);
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
    if (m_ControlThread.joinable())
    {
        m_ControlThread.join();
    }
}

void DataManWriter::ControlThread(const std::string &address)
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
