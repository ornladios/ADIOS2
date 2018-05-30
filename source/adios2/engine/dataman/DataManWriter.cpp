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

#include <iostream> //needs to go away, this is just for demo purposes

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManWriter::DataManWriter(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: DataManCommon("DataManWriter", io, name, mode, mpiComm), m_Name(name)
{
    m_EndMessage = ", in call to Open DataManWriter\n";
    Init();
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    ++m_CurrentStep;
    return StepStatus::OK;
}

void DataManWriter::EndStep()
{
    if (m_Format == "bp")
    {
        m_BP3Serializer->SerializeData(m_IO, true);
        m_BP3Serializer->CloseStream(m_IO);
        m_DataMan->WriteWAN(m_BP3Serializer->m_Data.m_Buffer);
        m_BP3Serializer->ResetBuffer(m_BP3Serializer->m_Data, true);
        m_BP3Serializer->ResetIndices();
    }
}

size_t DataManWriter::CurrentStep() const { return m_CurrentStep; }

// PRIVATE functions below

void DataManWriter::Init()
{
    if (m_Format == "bp")
    {
        m_BP3Serializer =
            std::make_shared<format::BP3Serializer>(m_MPIComm, m_DebugMode);
        m_BP3Serializer->InitParameters(m_IO.m_Parameters);
        m_BP3Serializer->PutProcessGroupIndex(m_IO.m_Name, m_IO.m_HostLanguage,
                                              {"WAN_Zmq"});
    }

    m_DataMan = std::make_shared<transportman::DataMan>(m_MPIComm, m_DebugMode);
    for (auto &i : m_IO.m_TransportsParameters)
    {
        i["TransportMode"] = m_TransportMode;
    }
    size_t channels = m_IO.m_TransportsParameters.size();
    std::vector<std::string> names;
    for (size_t i = 0; i < channels; ++i)
    {
        names.push_back(m_Name + std::to_string(i));
    }
    m_DataMan->OpenWANTransports(names, Mode::Write,
                                 m_IO.m_TransportsParameters, true);
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
            m_DataMan->WriteWAN(buffer);
        }
    }
}

} // end namespace adios2
