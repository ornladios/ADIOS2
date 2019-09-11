/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManWriter.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: Jason Wang
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
                             const Mode openMode, helper::Comm comm)
: DataManCommon("DataManWriter", io, name, openMode, std::move(comm))
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

        m_Comm.Allgather(m_DataAddress.data(), m_DataAddress.size(),
                         allDaVec.data(), 32);
        m_Comm.Allgather(m_ControlAddress.data(), m_ControlAddress.size(),
                         allCaVec.data(), 32);

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

    m_ReplyThread =
        std::thread(&DataManWriter::ReplyThread, this, m_ControlAddress);

    m_DataPublisher.OpenPublisher(m_DataAddress, m_Timeout);

    if (m_Reliable)
    {
        m_WriterSubAdios =
            std::make_shared<adios2::ADIOS>(mpiComm, adios2::DebugOFF);
        m_WriterSubIO = m_WriterSubAdios->DeclareIO("DataManWriter");
        m_WriterSubIO.SetEngine("bp4");
        m_WriterSubEngine =
            m_WriterSubIO.Open(m_Name, adios2::Mode::Write, mpiComm);

        m_ReaderSubAdios =
            std::make_shared<adios2::ADIOS>(mpiComm, adios2::DebugOFF);
        m_ReaderSubIO = m_ReaderSubAdios->DeclareIO("DataManReader");
        m_ReaderSubIO.SetEngine("bp4");
        m_ReaderSubEngine =
            m_ReaderSubIO.Open(m_Name, adios2::Mode::Read, MPI_COMM_SELF);
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
    m_FastSerializer.NewWriterBuffer(m_SerializerBufferSize);

    if (m_Reliable)
    {
        m_WriterSubEngine.BeginStep(mode, timeout_sec);
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::BeginStep() Rank " << m_MpiRank
                  << ", Step " << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

void DataManWriter::PerformPuts()
{
    if (m_Reliable)
    {
        m_WriterSubEngine.PerformPuts();
    }
}

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

    if (m_Reliable)
    {
        m_WriterSubEngine.EndStep();
    }
}

void DataManWriter::Flush(const int transportIndex)
{
    m_WriterSubEngine.Flush(transportIndex);
}

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

    if (m_Reliable)
    {
        m_WriterSubEngine.Close(transportIndex);
    }

    m_ThreadActive = false;
    if (m_ReplyThread.joinable())
    {
        m_ReplyThread.join();
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
            else if (r == "Attributes")
            {
                m_ReliableSerializer.PutAttributes(m_IO);
                m_ReliableSerializer.AttachAttributes();
                replier.SendReply(m_ReliableSerializer.GetLocalPack());
            }
            else
            {
                size_t step;
                try
                {
                    step = stoull(r);
                }
                catch (...)
                {
                    continue;
                }
                m_ReaderSubEngine.BeginStep();
                while (m_ReaderSubEngine.CurrentStep() < step)
                {
                    m_ReaderSubEngine.EndStep();
                    m_ReaderSubEngine.BeginStep();
                }
                auto varMap = m_ReaderSubIO.AvailableVariables();
                for (const auto &varPair : varMap)
                {
                    size_t elementBytes;
                    auto varParamsIt = varPair.second.find("Type");
                    std::string type;
                    if (varParamsIt == varPair.second.end())
                    {
                        throw("unknown data type");
                    }
                    else
                    {
                        type = varParamsIt->second;
                    }
                    if (type.empty())
                    {
                        throw("unknown data type");
                    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        ReadVarFromFile<T>(varPair.first);                                     \
    }
                    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
                    else { throw("unknown data type"); }
                }
                replier.SendReply(m_ReliableSerializer.GetLocalPack());
            }
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
