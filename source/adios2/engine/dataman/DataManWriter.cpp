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
    for (size_t i = 0; i < m_TransportChannels; ++i)
    {
        m_DataManSerializer[i]->New(m_BufferSize);
    }
    ++m_CurrentStep;
    return StepStatus::OK;
}

void DataManWriter::PerformPuts() {}

void DataManWriter::EndStep()
{
    if (m_Format == "bp")
    {
        m_BP3Serializer->SerializeData(m_IO, true);
        m_BP3Serializer->CloseStream(m_IO);
        m_DataMan->WriteWAN(m_BP3Serializer->m_Data.m_Buffer, 0);
        m_BP3Serializer->ResetBuffer(m_BP3Serializer->m_Data, true);
        m_BP3Serializer->ResetIndices();
    }
    else if (m_Format == "dataman")
    {
        for (size_t i = 0; i < m_TransportChannels; ++i)
        {
            const std::shared_ptr<std::vector<char>> buf =
                m_DataManSerializer[i]->Get();
            m_BufferSize = buf->size() * 2;
            m_DataMan->WriteWAN(buf, i);
        }
    }
    else if (m_Format == "binary")
    {
        throw(std::invalid_argument("[DataManWriter::EndStep] binary format is "
                                    "not supported in generic "
                                    "BeginStep-EndStep API."));
    }
    else
    {
        throw(std::invalid_argument("[DataManWriter::EndStep] format " +
                                    m_Format + " is not supported."));
    }
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

// PRIVATE functions below

void DataManWriter::Init()
{

    // initialize transports
    m_DataMan = std::make_shared<transportman::DataMan>(m_MPIComm, m_DebugMode);
    m_DataMan->OpenWANTransports(m_StreamNames, m_IO.m_TransportsParameters,
                                 Mode::Write, m_WorkflowMode, true);

    // initialize serializer
    if (m_Format == "bp")
    {
        m_BP3Serializer =
            std::make_shared<format::BP3Serializer>(m_MPIComm, m_DebugMode);
        m_BP3Serializer->InitParameters(m_IO.m_Parameters);
        m_BP3Serializer->PutProcessGroupIndex(m_IO.m_Name, m_IO.m_HostLanguage,
                                              {"WAN_Zmq"});
    }
    else if (m_Format == "dataman")
    {
        for (size_t i = 0; i < m_TransportChannels; ++i)
        {
            m_DataManSerializer.push_back(
                std::make_shared<format::DataManSerializer>(m_IsRowMajor,
                                                            m_IsLittleEndian));
        }
    }
    else if (m_Format == "binary")
    {
    }
    else
    {
        throw(std::invalid_argument("[DataManWriter::Init] format " + m_Format +
                                    " is not supported."));
    }
}

void DataManWriter::IOThread(std::shared_ptr<transportman::DataMan> man) {}

#define declare_type(T)                                                        \
    void DataManWriter::DoPutSync(Variable<T> &variable, const T *values)      \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void DataManWriter::DoPutDeferred(Variable<T> &variable, const T *values)  \
    {                                                                          \
        PutDeferredCommon(variable, values);                                   \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void DataManWriter::DoClose(const int transportIndex)
{
    if (m_Format == "bp")
    {
        m_BP3Serializer->CloseData(m_IO);
        auto &buffer = m_BP3Serializer->m_Data.m_Buffer;
        auto &position = m_BP3Serializer->m_Data.m_Position;
        if (position > 0)
        {
            m_DataMan->WriteWAN(buffer, transportIndex);
        }
    }
    else if (m_Format == "dataman")
    {
        m_DataMan->WriteWAN(format::DataManSerializer::EndSignal(CurrentStep()),
                            0);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
