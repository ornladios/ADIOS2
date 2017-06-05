/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "DataMan.h"

namespace adios
{
namespace transportman
{

DataMan::DataMan(MPI_Comm mpiComm, const bool debugMode)
: TransportMan(mpiComm, debugMode)
{
}

void DataMan::OpenWANTransports(const std::string &name,
                                const OpenMode openMode,
                                const std::vector<Params> &parametersVector,
                                const bool profile)
{
    auto lf_GetParameter = [](const std::string key, const Params &params,
                              const bool isMandatory,
                              const bool debugMode) -> std::string {

        std::string value;
        auto itParameter = params.find(key);
        if (itParameter == params.end())
        {
            if (debugMode && isMandatory)
            {
                throw std::invalid_argument(
                    "ERROR: wan transport doesn't have mandatory parameter " +
                    key +
                    ", provide one in IO AddTransport, in call to Open\n");
            }
        }
        else
        {
            value = itParameter->second;
        }
        return value;
    };

    for (const auto &parameters : parametersVector)
    {
        std::shared_ptr<Transport> wanTransport;

        const std::string type(
            lf_GetParameter("transport", parameters, true, m_DebugMode));

        const std::string lib(
            lf_GetParameter("lib", parameters, true, m_DebugMode));

        const std::string ipAddress(
            lf_GetParameter("ipaddress", parameters, true, m_DebugMode));

        const std::string port(
            lf_GetParameter("port", parameters, false, m_DebugMode));

        if (port.empty())
        {
            port = m_DefaultPort;
        }

        const std::string messageName(
            lf_GetParameter("name", parameters, false, m_DebugMode));

        if (messageName.empty())
        {
            messageName = name;
        }

        if (type == "wan") // need to create directory
        {
            if (lib == "zmq")
            {
#ifdef ADIOS_HAVE_ZMQ
                wanTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, port, m_MPIComm, m_DebugMode);
#else
                throw std::invalid_argument(
                    "ERROR: this version of ADIOS2 didn't compile with "
                    "ZMQ library, in call to Open\n");
#endif
            }
            else
            {
                if (m_DebugMode)
                {
                    throw std::invalid_argument("ERROR: wan library " + lib +
                                                " not supported or not "
                                                "provided in IO AddTransport, "
                                                "in call to Open\n");
                }
            }
        }
        wanTransport->Open(messageName, openMode);
        m_Transports.push_back(std::move(wanTransport));
    }
}

} // end namespace transportman
} // end namespace adios
