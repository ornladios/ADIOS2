/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include <fstream> //TODO go away

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

        const std::string TransportType(
            GetParameter("TransportType", parameters, true, m_DebugMode,
                         "Transport Type Parameter"));

        const std::string transport(GetParameter(
            "Transport", parameters, true, m_DebugMode, "Transport Parameter"));

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

        std::string transportName(GetParameter("Name", parameters, false,
                                               m_DebugMode,
                                               "Transport Name Parameter"));

        std::string format(
            GetParameter("Format", parameters, false, m_DebugMode, "Format"));
        if (format.empty())
        {
            format = "json";
        }

        if (transportName.empty())
        {
            transportName = name;
        }

        if (TransportType == "wan" || TransportType == "WAN")
        {
            if (transport == "zmq" || transport == "ZMQ")
            {
#ifdef ADIOS2_HAVE_ZEROMQ
                wanTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, portData, m_MPIComm, m_DebugMode);
                wanTransport->Open(transportName, mode);
                m_Transports.emplace(counter, wanTransport);

                controlTransport = std::make_shared<transport::WANZmq>(
                    ipAddress, portControl, m_MPIComm, m_DebugMode);
                controlTransport->Open(transportName, mode);
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
                    throw std::invalid_argument("ERROR: wan transport " +
                                                transport +
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
    if (m_CurrentTransport >= m_ControlTransports.size())
    {
        throw std::runtime_error("ERROR: No valid control transports found, "
                                 "from DataMan::WriteWAN()");
    }
    if (m_CurrentTransport >= m_Transports.size())
    {
        throw std::runtime_error(
            "ERROR: No valid transports found, from DataMan::WriteWAN()");
    }
    m_ControlTransports[m_CurrentTransport]->Write(jmsg.dump().c_str(),
                                                   jmsg.dump().size());
    m_Transports[m_CurrentTransport]->Write(
        reinterpret_cast<const char *>(buffer), jmsg["bytes"].get<size_t>());
}

void DataMan::WriteWAN(const void *buffer, size_t size)
{
    if (m_CurrentTransport >= m_Transports.size())
    {
        throw std::runtime_error(
            "ERROR: No valid transports found, from DataMan::WriteWAN()");
    }
    //    m_Transports[m_CurrentTransport]->Write(static_cast<const char
    //    *>(buffer),                                            size);

    m_Transports[m_CurrentTransport]->Write(
        reinterpret_cast<const char *>(buffer), size);

    std::ofstream bpfile("datamanW.bp", std::ios_base::binary);
    bpfile.write(reinterpret_cast<const char *>(buffer), size);
    bpfile.close();

    for (int i = 0; i < size / 4; i++)
    {

        std::cout << static_cast<const float *>(buffer)[i] << " ";
    }
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
    if (format == "json" || format == "JSON")
    {
        std::cout << "json" << std::endl;
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
            std::vector<char> buffer;
            buffer.reserve(m_BufferSize);

            Transport::Status status;
            trans->IRead(buffer.data(), m_BufferSize, status);

            if (status.Bytes > 0)
            {
                m_BP3Deserializer->m_Data.Resize(
                    status.Bytes, "in DataMan Streaming Listener");

                std::memcpy(m_BP3Deserializer->m_Data.m_Buffer.data(),
                            buffer.data(), status.Bytes);

                /*    write bp file for debugging   */
                /*
                std::ofstream bpfile("datamanR.bp", std::ios_base::binary);
                bpfile.write(m_BP3Deserializer->m_Data.m_Buffer.data(),
                             m_BP3Deserializer->m_Data.m_Buffer.size());
                bpfile.close();
                */

                m_BP3Deserializer->ParseMetadata(m_BP3Deserializer->m_Data,
                                                 *m_IO);

                const auto variablesInfo = m_IO->GetAvailableVariables();
                for (const auto &variableInfoPair : variablesInfo)
                {

                    std::string var = variableInfoPair.first;
                    std::string type = "null";
                    for (const auto &parameter : variableInfoPair.second)
                    {
                        std::cout << "\tKey: " << parameter.first
                                  << "\t Value: " << parameter.second << "\n";
                        if (parameter.first == "Type")
                        {
                            type = parameter.second;
                        }
                    }

                    if (type == "string")
                    {
                    }
                    else if (type == "char")
                    {
                    }
                    else if (type == "unsigned char")
                    {
                    }
                    else if (type == "short")
                    {
                    }
                    else if (type == "unsigned short")
                    {
                    }
                    else if (type == "int")
                    {
                        adios2::Variable<int> *v =
                            m_IO->InquireVariable<int>(var);
                        size_t size = std::accumulate(
                            v->m_Shape.begin(), v->m_Shape.end(), 1,
                            std::multiplies<size_t>());
                        std::vector<int> x(size);
                        v->SetData(x.data());
                        /* TODO: add read variable */
                        RunCallback(x.data(), "stream", var, type, v->m_Shape);
                    }
                    else if (type == "unsigned int")
                    {
                    }
                    else if (type == "long int")
                    {
                    }
                    else if (type == "unsigned long int")
                    {
                    }
                    else if (type == "long long int")
                    {
                    }
                    else if (type == "unsigned long long int")
                    {
                    }
                    else if (type == "float")
                    {
                    }
                    else if (type == "double")
                    {
                    }
                    else if (type == "long double")
                    {
                    }
                    else if (type == "float complex")
                    {
                    }
                    else if (type == "double complex")
                    {
                    }
                    else if (type == "long double complex")
                    {
                    }

                    std::cout << "Variable Name: " << var << std::endl;
                    std::cout << "Type: " << type << std::endl;
                }
            }
        }
    }
}

void DataMan::RunCallback(void *buffer, std::string doid, std::string var,
                          std::string dtype, std::vector<size_t> shape)
{
    if (m_Callback != nullptr && m_Callback->m_Type == "Signature2")
    {
        m_Callback->RunCallback2(buffer, doid, var, dtype, shape);
    }
}

} // end namespace transportman
} // end namespace adios2
