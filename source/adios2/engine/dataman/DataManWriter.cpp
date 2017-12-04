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
: Engine("DataManWriter", io, name, mode, mpiComm),
  m_BP3Serializer(mpiComm, m_DebugMode), m_Man(mpiComm, m_DebugMode)
{
    m_EndMessage = ", in call to Open DataManWriter\n";
    Init();
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    return StepStatus::OK;
}
void DataManWriter::EndStep() { m_BP3Serializer.SerializeData(m_IO, true); }

void DataManWriter::Close(const int transportIndex)
{
    if (m_UseFormat == "bp" || m_UseFormat == "BP")
    {
        m_BP3Serializer.CloseData(m_IO);
        auto &buffer = m_BP3Serializer.m_Data.m_Buffer;
        auto &position = m_BP3Serializer.m_Data.m_Position;
        if (position > 0)
        {
            m_Man.WriteWAN(buffer.data(), position);
        }
    }
}

// PRIVATE functions below
void DataManWriter::Init()
{

    auto lf_GetBoolParameter = [&](const std::string key, bool &value) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            if (itKey->second == "yes" || itKey->second == "true")
            {
                value = true;
            }
            else if (itKey->second == "no" || itKey->second == "false")
            {
                value = false;
            }
        }
    };

    auto lf_GetStringParameter = [&](const std::string key,
                                     std::string &value) {
        auto it = m_IO.m_Parameters.find(key);
        if (it != m_IO.m_Parameters.end())
        {
            value = it->second;
        }
    };

    auto lf_GetUIntParameter = [&](const std::string key, unsigned int &value) {
        auto it = m_IO.m_Parameters.find(key);
        if (it != m_IO.m_Parameters.end())
        {
            value = std::stoi(it->second);
        }
    };

    lf_GetBoolParameter("Monitoring", m_DoMonitor);
    lf_GetUIntParameter("NTransports", m_NTransports);
    std::string TransportType;
    lf_GetStringParameter("TransportType", TransportType);
    std::string Transport;
    lf_GetStringParameter("Transport", Transport);

    // Check if using BP Format and initialize buffer
    lf_GetStringParameter("Format", m_UseFormat);

    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        m_BP3Serializer.InitParameters(m_IO.m_Parameters);
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_HostLanguage, {"WAN_Zmq"});
    }

    if (TransportType.empty() && Transport.empty())
    {
        throw std::runtime_error(
            "ERROR: No transports specified in user application!");
    }

    std::vector<Params> parameters(m_NTransports);
    for (unsigned int i = 0; i < parameters.size(); i++)
    {
        parameters[i]["TransportType"] = TransportType;
        parameters[i]["Transport"] = "zmq";
        parameters[i]["name"] = "stream";
        parameters[i]["IPAddress"] = "127.0.0.1";
        parameters[i]["Format"] = m_UseFormat;
    }

    m_Man.OpenWANTransports("zmq", Mode::Write, parameters, true);
}

#define declare_type(T)                                                        \
    void DataManWriter::DoPutSync(Variable<T> &variable, const T *values)      \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
