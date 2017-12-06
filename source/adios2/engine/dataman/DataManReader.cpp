/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: wfg
 */

#include "DataManReader.h"

#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManReader::DataManReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("DataManReader", io, name, mode, mpiComm),
  m_BP3Deserializer(mpiComm, m_DebugMode), m_Man(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

void DataManReader::Close(const int transportIndex) {}

// PRIVATE
void DataManReader::Init()
{
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

    for (auto &j : m_IO.m_Operators)
    {
        if (j.ADIOSOperator.m_Type == "Signature2")
        {
            m_Man.SetCallback(j.ADIOSOperator);
            break;
        }
    }

    lf_GetUIntParameter("NTransports", m_NTransports);
    std::vector<Params> parameters(m_NTransports);

    std::string TransportType;
    lf_GetStringParameter("TransportType", TransportType);

    std::string Transport;
    lf_GetStringParameter("Transport", Transport);

    // Check if using BP Format and initialize buffer

    lf_GetStringParameter("Format", m_UseFormat);

    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        m_BP3Deserializer.InitParameters(m_IO.m_Parameters);
    }

    for (unsigned int i = 0; i < parameters.size(); i++)
    {
        parameters[i]["TransportType"] = TransportType;
        parameters[i]["Transport"] = Transport;
        parameters[i]["Name"] = "stream";
        parameters[i]["IPAddress"] = "127.0.0.1";
        parameters[i]["Format"] = m_UseFormat;
    }

    m_Man.SetBP3Deserializer(m_BP3Deserializer);
    m_Man.SetIO(m_IO);
    m_Man.OpenWANTransports("zmq", Mode::Read, parameters, true);
}

} // end namespace adios2
