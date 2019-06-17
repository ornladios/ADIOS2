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
        m_DataManSerializer[0]->AggregateMetadata();
        m_AggregatedMetadataMutex.lock();
        int64_t stepProvided;
        m_AggregatedMetadata =
            m_DataManSerializer[0]->GetAggregatedMetadataPack(0, stepProvided,
                                                              -1);
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
                m_IsRowMajor, m_ContiguousMajor, m_IsLittleEndian, m_MPIComm));
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
        if (request && request->size() > 0)
        {
            std::lock_guard<std::mutex> lck(m_AggregatedMetadataMutex);
            tpm.SendReply(m_AggregatedMetadata);
        }
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
