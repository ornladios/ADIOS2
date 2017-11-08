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
: Engine("DataManWriter", io, name, mode, mpiComm), m_Man(mpiComm, m_DebugMode)
{
    m_EndMessage = ", in call to Open DataManWriter\n";
    Init();
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    return StepStatus::OK;
}
void DataManWriter::EndStep() {}

void DataManWriter::Close(const int transportIndex) {}

// PRIVATE functions below
void DataManWriter::Init()
{
    auto lf_SetBoolParameter = [&](const std::string key, bool &parameter) {

        auto itKey = m_IO.m_Parameters.find(key);
        if (itKey != m_IO.m_Parameters.end())
        {
            if (itKey->second == "yes" || itKey->second == "true")
            {
                parameter = true;
            }
            else if (itKey->second == "no" || itKey->second == "false")
            {
                parameter = false;
            }
        }
    };

    lf_SetBoolParameter("real_time", m_DoRealTime);
    lf_SetBoolParameter("monitoring", m_DoMonitor);

    if (m_DoRealTime)
    {
        /**
         * Lambda function that assigns a parameter in m_Method to a
         * localVariable
         * of type std::string
         */
        auto lf_AssignString = [&](const std::string parameter,
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
        auto lf_AssignInt = [&](const std::string parameter,
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
        unsigned int n_Transports = 1;
        std::vector<Params> parameters(n_Transports);

        for (unsigned int i = 0; i < parameters.size(); i++)
        {
            parameters[i]["type"] = "wan";
            parameters[i]["transport"] = "zmq";
            parameters[i]["name"] = "stream";
            parameters[i]["ipaddress"] = "127.0.0.1";
        }

        m_Man.OpenWANTransports("zmq", Mode::Write, parameters, true);

        std::string method_type;
        lf_AssignString("method_type", method_type);

        int num_channels = 0;
        lf_AssignInt("num_channels", num_channels);
    }
}

#define declare_type(T)                                                        \
    void DataManWriter::DoPutSync(Variable<T> &variable, const T *values)      \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
