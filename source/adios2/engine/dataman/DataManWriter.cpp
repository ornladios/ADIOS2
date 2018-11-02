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
    ++m_CurrentStep;
    if (m_Format == "dataman")
    {
        for (size_t i = 0; i < m_TransportChannels; ++i)
        {
            m_DataManSerializer[i]->New(m_BufferSize);
        }
    }
    else if (m_Format == "binary")
    {
    }
    else
    {
        throw(std::invalid_argument("[DataManWriter::EndStep] format " +
                                    m_Format + " is not supported."));
    }
    return StepStatus::OK;
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

void DataManWriter::PerformPuts() {}

void DataManWriter::EndStep()
{
    if (m_Format == "dataman")
    {
        for (size_t i = 0; i < m_TransportChannels; ++i)
        {
            if (m_CurrentStep == 0)
            {
                m_DataManSerializer[i]->PutAttributes(m_IO, m_MPIRank);
            }
            const std::shared_ptr<std::vector<char>> buf =
                m_DataManSerializer[i]->Get();
            m_BufferSize = buf->size() * 2;
            m_DataMan->WriteWAN(buf, i);
        }
    }
    else if (m_Format == "binary")
    {
    }
    else
    {
        throw(std::invalid_argument("[DataManWriter::EndStep] format " +
                                    m_Format + " is not supported."));
    }
}

void DataManWriter::Flush(const int transportIndex) {}

// PRIVATE functions below

void DataManWriter::Init()
{

    // initialize transports
    m_DataMan = std::make_shared<transportman::DataMan>(m_MPIComm, m_DebugMode);
    m_DataMan->OpenWANTransports(m_StreamNames, m_IO.m_TransportsParameters,
                                 Mode::Write, m_WorkflowMode, true);

    // initialize serializer
    if (m_Format == "dataman")
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
    if (m_Format == "dataman")
    {
        m_DataMan->WriteWAN(format::DataManSerializer::EndSignal(CurrentStep()),
                            0);
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
