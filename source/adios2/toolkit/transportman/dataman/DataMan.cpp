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

#include "adios2/helper/adiosFunctions.h"

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

DataMan::~DataMan()
{
    for (auto &controlThread : m_ControlThreads)
    {
        m_Listening = false;
        controlThread.join();
    }
}

void DataMan::OpenWANTransports(const std::string &name, const Mode mode,
                                const std::vector<Params> &parametersVector,
                                const bool profile)
{
#ifdef ADIOS2_HAVE_ZEROMQ
    size_t counter = 0; // remove MACRO when more libraries are added
#endif

    for (const auto &parameters : parametersVector)
    {
        std::shared_ptr<Transport> wanTransport, controlTransport;

        const std::string type(GetParameter(
            "type", parameters, true, m_DebugMode, "Transport Type Parameter"));

        const std::string library(GetParameter("Library", parameters, true,
                                               m_DebugMode,
                                               "Transport Library Parameter"));

        const std::string ipAddress(
            GetParameter("IPAddress", parameters, true, m_DebugMode,
                         "Transport IPAddress Parameter"));

        std::string portControl(GetParameter("Port", parameters, false,
                                             m_DebugMode,
                                             "Transport Port Parameter"));

        if (portControl.empty())
        {
            portControl = std::to_string(m_DefaultPort);
        }

        const std::string portData(std::to_string(stoi(portControl) + 1));

        std::string messageName(GetParameter("Name", parameters, false,
                                             m_DebugMode,
                                             "Transport Name Parameter"));

        std::string format(
            GetParameter("Format", parameters, false, m_DebugMode, "Format"));
        if (format.empty())
        {
            format = "json";
        }

        if (messageName.empty())
        {
            messageName = name;
        }

        if (type == "wan" || type == "WAN")
        {
            if (library == "zmq" || library == "ZMQ")
            {
#ifdef ADIOS2_HAVE_ZEROMQ
                wanTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, portData, m_MPIComm, m_DebugMode);
                wanTransport->Open(messageName, mode);
                m_Transports.emplace(counter, wanTransport);

                controlTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, portControl, m_MPIComm, m_DebugMode);
                controlTransport->Open(messageName, mode);
                m_ControlTransports.emplace_back(controlTransport);

                if (mode == Mode::Read)
                {
                    m_Listening = true;
                    m_ControlThreads.emplace_back(
                        std::thread(&DataMan::ReadThread, this, wanTransport,
                                    controlTransport, format));
                }
                ++counter;

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
                    throw std::invalid_argument("ERROR: wan library " +
                                                library +
                                                " not supported or not "
                                                "provided in IO AddTransport, "
                                                "in call to Open\n");
                }
            }
        }
    }
}

void DataMan::WriteWAN(const void *buffer, nlohmann::json jmsg)
{
    m_ControlTransports[m_CurrentTransport]->Write(jmsg.dump().c_str(),
                                                   jmsg.dump().size());
    m_Transports[m_CurrentTransport]->Write(static_cast<const char *>(buffer),
                                            jmsg["bytes"].get<size_t>());
}

void DataMan::WriteWAN(const char *buffer, size_t size)
{
    m_Transports[m_CurrentTransport]->Write(buffer, size);
}

void DataMan::ReadWAN(void *buffer, nlohmann::json jmsg) {}

void DataMan::SetBP3Deserializer(format::BP3Deserializer &bp3Deserializer)
{
    m_BP3Deserializer = &bp3Deserializer;
}

void DataMan::SetIO(IO &io) { m_IO = &io; }

void DataMan::SetCallback(adios2::Operator &callback)
{
    m_Callback = &callback;
}

void DataMan::ReadThread(std::shared_ptr<Transport> trans,
                         std::shared_ptr<Transport> ctl_trans,
                         const std::string format)
{
    if (format == "json")
    {
        while (m_Listening)
        {
            char buffer[1024];
            size_t bytes = 0;
            nlohmann::json jmsg;
            adios2::Transport::Status status;
            ctl_trans->IRead(buffer, 1024, status, 0);
            if (status.Bytes > 0)
            {
                std::string smsg(buffer);
                jmsg = nlohmann::json::parse(smsg);
                bytes = jmsg.value("bytes", 0);
                if (bytes > 0)
                {
                    std::vector<char> data(bytes);
                    trans->Read(data.data(), bytes);
                    std::string doid =
                        jmsg.value("doid", "Unknown Data Object");
                    std::string var = jmsg.value("var", "Unknown Variable");
                    std::string dtype =
                        jmsg.value("dtype", "Unknown Data Type");
                    std::vector<size_t> putshape =
                        jmsg.value("putshape", std::vector<size_t>());
                    if (m_Callback != nullptr &&
                        m_Callback->m_Type == "Signature2")
                    {
                        m_Callback->RunCallback2(data.data(), doid, var, dtype,
                                                 putshape);
                    }
                }
            }
        }
    }
    else if (format == "bp" || format == "BP")
    {
        while (m_Listening)
        {
            uint64_t bufferSize = 0;
            trans->Read(reinterpret_cast<char *>(&bufferSize), 8);

            if (bufferSize > 0)
            {
                size_t size = static_cast<size_t>(bufferSize);
                m_BP3Deserializer->m_Data.Resize(
                    size, "in DataMan Streaming Listener");

                trans->Read(m_BP3Deserializer->m_Data.m_Buffer.data(), size);
                m_BP3Deserializer->ParseMetadata(*m_IO);

                const auto variablesInfo = m_IO->GetAvailableVariables();
                for (const auto &variableInfoPair : variablesInfo)
                {
                    std::cout << "Variable Name: " << variableInfoPair.first
                              << "\n";

                    for (const auto &parameter : variableInfoPair.second)
                    {
                        std::cout << "\tKey: " << parameter.first
                                  << "\t Value: " << parameter.second << "\n";
                    }
                }
            }
        }
    }
}

} // end namespace transportman
} // end namespace adios2
