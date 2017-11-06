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
: Engine("DataManReader", io, name, mode, mpiComm), m_Man(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

void DataManReader::Close(const int transportIndex) {}

// PRIVATE
void DataManReader::Init()
{
    auto itRealTime = m_IO.m_Parameters.find("real_time");
    if (itRealTime != m_IO.m_Parameters.end())
    {
        if (itRealTime->second == "yes" || itRealTime->second == "true")
            m_DoRealTime = true;
    }

    if (m_DoRealTime)
    {
        /**
         * Lambda function that assigns a parameter in m_Method to a
         * localVariable
         * of type std::string
         */
        auto lf_AssignString = [this](const std::string parameter,
                                      std::string &localVariable) {
            auto it = m_IO.m_Parameters.find(parameter);
            if (it != m_IO.m_Parameters.end())
            {
                localVariable = it->second;
            }
        };

        /**
         * Lambda function that assigns a parameter in m_Method to a
         * localVariable
         * of type int
         */
        auto lf_AssignInt = [this](const std::string parameter,
                                   int &localVariable) {
            auto it = m_IO.m_Parameters.find(parameter);
            if (it != m_IO.m_Parameters.end())
            {
                localVariable = std::stoi(it->second);
            }
        };

        // try not to hardcode these things...
        // shouldn't these be coming from the user IO.m_TransportParameters?
        // unless you assign defaults
        unsigned int transportsSize = 1;

        std::vector<Params> parameters(transportsSize);

        for (unsigned int i = 0; i < parameters.size(); i++)
        {
            parameters[i]["type"] = "wan";
            parameters[i]["transport"] = "zmq";
            parameters[i]["name"] = "stream";
            parameters[i]["ipaddress"] = "127.0.0.1";
        }
        m_Man.OpenWANTransports("zmq", Mode::Read, parameters, true);

        std::string methodType;
        int numChannels = 0;
        lf_AssignString("method_type", methodType);
        lf_AssignInt("num_channels", numChannels);
    }
    else
    {
        InitTransports();
    }
}

} // end namespace adios2
