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

#include "adios2/ADIOSMacros.h"
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
    for (auto &readThread : m_ReadThreads)
    {
        m_Listening = false;
        readThread.join();
    }
}

void DataMan::OpenWANTransports(const std::vector<std::string> &streamNames,
                                const Mode mode,
                                const std::vector<Params> &paramsVector,
                                const bool profile)
{
#ifdef ADIOS2_HAVE_ZEROMQ
    size_t counter = 0; // remove MACRO when more libraries are added
#endif

    if (streamNames.size() == 0)
    {
        throw("No streams to open from DataMan::OpenWANTransports");
    }

    for (size_t i = 0; i < streamNames.size(); ++i)
    {

        std::shared_ptr<Transport> wanTransport, controlTransport;

        const std::string library(GetParameter("Library", paramsVector[i], true,
                                               m_DebugMode,
                                               "Transport Library Parameter"));

        const std::string callback(
            GetParameter("Callback", paramsVector[i], false, m_DebugMode,
                         "Transport Callback Parameter"));

        const std::string ipAddress(
            GetParameter("IPAddress", paramsVector[i], true, m_DebugMode,
                         "Transport IPAddress Parameter"));

        std::string port(GetParameter("Port", paramsVector[i], false,
                                      m_DebugMode, "Transport Port Parameter"));

        if (port.empty())
        {
            port = std::to_string(m_DefaultPort);
        }

        int mpiRank;
        MPI_Comm_rank(m_MPIComm, &mpiRank);

        port = std::to_string(stoi(port) + mpiRank);

        if (library == "zmq" || library == "ZMQ")
        {
#ifdef ADIOS2_HAVE_ZEROMQ
            wanTransport = std::make_shared<transport::WANZmq>(
                ipAddress, port, m_MPIComm, m_DebugMode);
            wanTransport->Open(streamNames[i], mode);
            m_Transports.emplace(counter, wanTransport);

            if (mode == Mode::Read)
            {
                if (callback == "YES" || callback == "yes" ||
                    callback == "TRUE" || callback == "true")
                {
                    m_Listening = true;
                    m_ReadThreads.emplace_back(
                        std::thread(&DataMan::ReadThread, this, wanTransport,
                                    streamNames[i], paramsVector[i]));
                }
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
                throw std::invalid_argument("ERROR: wan transport " + library +
                                            " not supported or not "
                                            "provided in IO AddTransport, "
                                            "in call to Open\n");
            }
        }
    }
}

void DataMan::WriteWAN(const void *buffer, size_t size)
{
    if (m_CurrentTransport >= m_Transports.size())
    {
        throw std::runtime_error(
            "ERROR: No valid transports found, from DataMan::WriteWAN()");
    }

    m_Transports[m_CurrentTransport]->Write(
        reinterpret_cast<const char *>(buffer), size);
}

void DataMan::ReadWAN(void *buffer, size_t &size)
{

    Transport::Status status;
    for (int i = 0; i < m_Timeout * 1000; ++i)
    {
        m_Transports[m_CurrentTransport]->IRead(static_cast<char *>(buffer),
                                                m_BufferSize, status);
        if (status.Bytes > 0)
        {
            size = status.Bytes;
            return;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
}

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
                         const std::string stream_name,
                         const Params trans_params)
{
    std::string format(GetParameter("Format", trans_params, false, m_DebugMode,
                                    "Transport Format Parameter"));

    if (format.empty())
    {
        format = "bp";
    }

    if (format == "bp" || format == "BP")
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

                const std::string dumpFile(
                    GetParameter("DumpFile", trans_params, false, m_DebugMode,
                                 "Transport DumpFile Parameter"));

                // TODO: Aggregation
                if (dumpFile == "yes" || dumpFile == "YES" ||
                    dumpFile == "TRUE" || dumpFile == "true")
                {
                    std::ofstream bpfile(stream_name, std::ios_base::binary);
                    bpfile.write(reinterpret_cast<const char *>(buffer.data()),
                                 status.Bytes);
                    bpfile.close();
                }

                m_BP3Deserializer->ParseMetadata(m_BP3Deserializer->m_Data,
                                                 *m_IO);

                const auto variablesInfo = m_IO->GetAvailableVariables();
                for (const auto &variableInfoPair : variablesInfo)
                {

                    std::string var = variableInfoPair.first;
                    std::string type = "null";

                    for (const auto &parameter : variableInfoPair.second)
                    {
                        //  ** print out all parameters from BP metadata
                        /*
                            std::cout << "\tKey: " << parameter.first
                                      << "\t Value: " << parameter.second <<
                           "\n";
                        */
                        if (parameter.first == "Type")
                        {
                            type = parameter.second;
                        }
                    }

                    if (type == "compound")
                    {
                        // not supported
                    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        adios2::Variable<T> *v = m_IO->InquireVariable<T>(var);                \
        m_BP3Deserializer->GetSyncVariableDataFromStream(                      \
            *v, m_BP3Deserializer->m_Data);                                    \
        if (v->GetData() == nullptr)                                           \
        {                                                                      \
            throw("Data pointer obtained from BP deserializer is a nullptr");  \
        }                                                                      \
        RunCallback(v->GetData(), "stream", var, type, v->m_Shape);            \
    }
                    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
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
