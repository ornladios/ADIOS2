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

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{
namespace core
{
namespace engine
{

DataManWriter::DataManWriter(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: DataManCommon("DataManWriter", io, name, mode, mpiComm)
{
    m_EndMessage = ", in call to Open DataManWriter\n";
    Init();
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::BeginStep() begin. Last step "
                  << m_CurrentStep << std::endl;
    }
    ++m_CurrentStep;

    for (size_t i = 0; i < m_Channels; ++i)
    {
        m_DataManSerializer[i]->New(m_BufferSize);
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManWriter::BeginStep() end. Current step "
                  << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

void DataManWriter::PerformPuts() {}

void DataManWriter::EndStep()
{
    for (auto &serializer : m_DataManSerializer)
    {
        serializer->PutAttributes(m_IO);
    }

    if (m_CurrentStep == 0)
    {
        m_DataManSerializer[0]->AggregateMetadata(m_MPIComm);
        m_AggregatedMetadataMutex.lock();
        m_AggregatedMetadata =
            m_DataManSerializer[0]->GetAggregatedMetadataPack(0);
        m_AggregatedMetadataMutex.unlock();
    }

    if (m_WorkflowMode == "file")
    {
        const auto buf = m_DataManSerializer[0]->GetLocalPack();
        m_FileTransport.Write(buf->data(), buf->size());
    }
    else if (m_WorkflowMode == "stream")
    {
        for (size_t i = 0; i < m_Channels; ++i)
        {
            m_DataManSerializer[i]->AttachAttributes();
            const auto buf = m_DataManSerializer[i]->GetLocalPack();
            m_BufferSize = buf->size();
            m_WANMan->Write(buf, i);
        }
    }
}

void DataManWriter::Flush(const int transportIndex) {}

// PRIVATE functions below

void DataManWriter::Init()
{

    if (m_WorkflowMode == "file")
    {
        m_FileTransport.Open(m_Name, Mode::Write);
        return;
    }

    // initialize transports
    m_WANMan = std::make_shared<transportman::WANMan>(m_MPIComm, m_DebugMode);
    m_WANMan->OpenTransports(m_IO.m_TransportsParameters, Mode::Write,
                             m_WorkflowMode, true);

    // initialize serializer
    for (size_t i = 0; i < m_Channels; ++i)
    {
        m_DataManSerializer.push_back(
            std::make_shared<format::DataManSerializer>(
                m_IsRowMajor, m_ContiguousMajor, m_IsLittleEndian));
    }
}

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
    if (m_WorkflowMode == "file")
    {
        m_FileTransport.Close();
        return;
    }

    m_WANMan->Write(format::DataManSerializer::EndSignal(CurrentStep()), 0);
}

void DataManWriter::MetadataThread(const std::string &address)
{
    transportman::StagingMan tpm(m_MPIComm, Mode::Write, 0, 1e7);
    tpm.OpenTransport(address);
    while (m_Listening)
    {
        auto request = tpm.ReceiveRequest();
        if (request == nullptr)
        {
            continue;
        }
        if (request->size() >= 0)
        {
            m_AggregatedMetadataMutex.lock();
            tpm.SendReply(m_AggregatedMetadata);
            m_AggregatedMetadataMutex.unlock();
        }
    }
}

void DataManWriter::Handshake()
{
    // Get IP address
    auto ips = helper::AvailableIpAddresses();
    std::string ip = "127.0.0.1";
    if (ips.empty() == false)
    {
        ip = ips[0];
    }

    // Check total number of writer apps
    if (m_MpiRank == 0)
    {
        transport::FileFStream lockCheck(m_MPIComm, m_DebugMode);
        while (true)
        {
            try
            {
                lockCheck.Open(".wdm.lock", Mode::Read);
                lockCheck.Close();
            }
            catch (...)
            {
                break;
            }
        }
        transport::FileFStream lockWrite(m_MPIComm, m_DebugMode);
        lockWrite.Open(".wdm.lock", Mode::Write);

        transport::FileFStream numRead(m_MPIComm, m_DebugMode);
        try
        {
            numRead.Open(".wdm", Mode::Read);
            auto size = numRead.GetSize();
            std::vector<char> numAppsChar(size);
            numRead.Read(numAppsChar.data(), numAppsChar.size());
            m_AppID =
                1 + stoi(std::string(numAppsChar.begin(), numAppsChar.end()));
            numRead.Close();
        }
        catch (...)
        {
        }
        transport::FileFStream numWrite(m_MPIComm, m_DebugMode);
        numWrite.Open(".wdm", Mode::Write);
        std::string numAppsString = std::to_string(m_AppID);
        numWrite.Write(numAppsString.data(), numAppsString.size());
        numWrite.Close();

        lockWrite.Close();
        remove(".wdm.lock");
    }

    // Make full addresses
    for (int i = 0; i < m_Channels; ++i)
    {
        std::string addr = "tcp://" + ip + ":" +
                           std::to_string(m_Port + (100 * m_AppID) +
                                          (m_MpiRank % 1000) * m_Channels + i) +
                           "\0";
        m_FullAddresses.push_back(addr);
    }
    nlohmann::json localAddressesJson = m_FullAddresses;
    std::string localAddressesStr = localAddressesJson.dump();
    std::vector<char> localAddressesChar(64 * m_Channels, '\0');
    std::memcpy(localAddressesChar.data(), localAddressesStr.c_str(),
                localAddressesStr.size());
    std::vector<char> globalAddressesChar(64 * m_Channels * m_MpiSize, '\0');
    helper::GatherArrays(localAddressesChar.data(), 64 * m_Channels,
                         globalAddressesChar.data(), m_MPIComm);

    // Writing handshake file
    if (m_MpiRank == 0)
    {
        nlohmann::json globalAddressesJson;
        for (int i = 0; i < m_MpiSize; ++i)
        {
            auto j = nlohmann::json::parse(
                &globalAddressesChar[i * 64 * m_Channels]);
            for (auto &i : j)
            {
                globalAddressesJson.push_back(i);
            }
        }
        std::string globalAddressesStr = globalAddressesJson.dump();
        transport::FileFStream lockstream(m_MPIComm, m_DebugMode);
        lockstream.Open(m_Name + ".wdm.lock", Mode::Write);
        transport::FileFStream ipstream(m_MPIComm, m_DebugMode);
        ipstream.Open(m_Name + ".wdm", Mode::Write);
        ipstream.Write(globalAddressesStr.data(), globalAddressesStr.size());
        ipstream.Close();
        lockstream.Close();
        remove(std::string(m_Name + ".wdm.lock").c_str());
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
