/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "adios2/toolkit/transportman/dataman/DataMan.h"
#include "adios2/helper/adiosString.h"

#ifdef ADIOS2_HAVE_ZEROMQ
#include "adios2/toolkit/transport/wan/WANZmq.h"
#endif

namespace adios2
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

    for (const auto &parameters : parametersVector)
    {
        std::shared_ptr<Transport> wanTransport, controlTransport;

        // to be removed
        for (auto &i : parameters)
        {
            std::cout << i.first << " " << i.second << std::endl;
        }

        const std::string type(
            GetParameter("type", parameters, true, m_DebugMode, ""));

        const std::string trans(
            GetParameter("transport", parameters, true, m_DebugMode, ""));

        const std::string ipAddress(
            GetParameter("ipaddress", parameters, true, m_DebugMode, ""));

        std::string port_control(
            GetParameter("port", parameters, false, m_DebugMode, ""));

        if (port_control.empty())
        {
            port_control = std::to_string(m_DefaultPort);
        }

        const std::string port_data(std::to_string(stoi(port_control) + 1));

        std::string messageName(
            GetParameter("name", parameters, false, m_DebugMode, ""));

        if (messageName.empty())
        {
            messageName = name;
        }

        if (type == "wan") // need to create directory
        {
            if (trans == "zmq")
            {
#ifdef ADIOS2_HAVE_ZEROMQ
                wanTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, port_data, m_MPIComm, m_DebugMode);
                controlTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, port_control, m_MPIComm, m_DebugMode);
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
                    throw std::invalid_argument("ERROR: wan library " + trans +
                                                " not supported or not "
                                                "provided in IO AddTransport, "
                                                "in call to Open\n");
                }
            }
        }

        wanTransport->Open(messageName, openMode);
        m_Transports.push_back(std::move(wanTransport));
        controlTransport->Open(messageName, openMode);
        m_ControlTransports.push_back(std::move(controlTransport));
    }
}

void DataMan::WriteWAN(const void *buffer, nlohmann::json jmsg)
{
    m_ControlTransports[m_CurrentTransport]->Write(jmsg.dump().c_str(),
                                                   jmsg.dump().size());
    m_Transports[m_CurrentTransport]->Write(static_cast<const char *>(buffer),
                                            jmsg["bytes"].get<size_t>());
}

void DataMan::SetCallback(std::function<void(const void *, std::string,
                                             std::string, std::string, Dims)>
                              callback)
{
    m_CallBack = callback;
}

void DataMan::ReadThread(std::shared_ptr<Transport> trans,
                         std::shared_ptr<Transport> ctl_trans)
{
    while (m_Listening)
    {
        // Wait for Read API to be implemented
        /*
        if (ctl_trans->Read() >= 0)
        {
            std::string smsg;
            nlohmann::json jmsg = json::parse(smsg);
        }
        else
        {
            usleep(1);
        }
        */
    }
}

} // end namespace transportman
} // end namespace adios
