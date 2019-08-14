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

    m_DataManSerializer.NewWriterBuffer(m_BufferSize);

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
    m_DataManSerializer.PutAttributes(m_IO);

    if (m_CurrentStep == 0)
    {
        m_DataManSerializer.AggregateMetadata();
        m_AggregatedMetadataMutex.lock();
        int64_t stepProvided;
        m_AggregatedMetadata =
            m_DataManSerializer.GetAggregatedMetadataPack(0, stepProvided, -1);
        m_AggregatedMetadataMutex.unlock();
    }

    m_DataManSerializer.AttachAttributes();
    const auto buf = m_DataManSerializer.GetLocalPack();
    m_BufferSize = buf->size();
    m_WANMan.Write(buf);
}

void DataManWriter::Flush(const int transportIndex) {}

// PRIVATE functions below

void DataManWriter::Init()
{
    m_WANMan.OpenTransports(m_IO.m_TransportsParameters, Mode::Write, true);
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
    m_WANMan.Write(format::DataManSerializer::EndSignal(CurrentStep()), 0);
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
