/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include <fstream>  //TODO go away
#include <iostream> //TODO go away

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

void DataMan::SetMaxReceiveBuffer(size_t size) { m_MaxReceiveBuffer = size; }

void DataMan::OpenWANTransports(const std::vector<std::string> &streamNames,
                                const Mode mode,
                                const std::vector<Params> &paramsVector,
                                const bool profile)
{

    if (streamNames.size() == 0)
    {
        std::cout << streamNames.size() << std::endl;
        throw("No streams to open from DataMan::OpenWANTransports");
    }

    for (size_t i = 0; i < streamNames.size(); ++i)
    {

        // Get parameters
        const std::string library(GetParameter("Library", paramsVector[i], true,
                                               m_DebugMode,
                                               "Transport Library Parameter"));

        const std::string ipAddress(
            GetParameter("IPAddress", paramsVector[i], true, m_DebugMode,
                         "Transport IPAddress Parameter"));

        std::string port(GetParameter("Port", paramsVector[i], false,
                                      m_DebugMode, "Transport Port Parameter"));

        // Calculate port number
        if (port.empty())
        {
            port = std::to_string(m_DefaultPort);

            int mpiRank, mpiSize;
            MPI_Comm_rank(m_MPIComm, &mpiRank);
            MPI_Comm_size(m_MPIComm, &mpiSize);

            port = std::to_string(stoi(port) + mpiRank + i * mpiSize);
        }

        std::shared_ptr<Transport> wanTransport;

        if (library == "zmq" || library == "ZMQ")
        {
#ifdef ADIOS2_HAVE_ZEROMQ

            wanTransport = std::make_shared<transport::WANZmq>(
                ipAddress, port, m_MPIComm, m_DebugMode);
            wanTransport->Open(streamNames[i], mode);
            m_Transports.emplace(i, wanTransport);

            if (mode == Mode::Read)
            {

                m_Listening = true;
                m_ReadThreads.emplace_back(
                    std::thread(&DataMan::ReadThread, this, wanTransport));
            }

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

void DataMan::WriteWAN(const std::vector<char> &buffer, size_t size)
{
    if (m_CurrentTransport >= m_Transports.size())
    {
        throw std::runtime_error(
            "ERROR: No valid transports found, from DataMan::WriteWAN()");
    }

    m_Transports[m_CurrentTransport]->Write(
        reinterpret_cast<const char *>(buffer.data()), size);
}

std::shared_ptr<std::vector<char>> DataMan::ReadWAN()
{
    return PopBufferQueue();
}

void DataMan::PushBufferQueue(std::shared_ptr<std::vector<char>> v)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    m_BufferQueue.push(v);
}

std::shared_ptr<std::vector<char>> DataMan::PopBufferQueue()
{
    std::lock_guard<std::mutex> l(m_Mutex);
    if (m_BufferQueue.size() > 0)
    {
        std::shared_ptr<std::vector<char>> vec = m_BufferQueue.front();
        m_BufferQueue.pop();
        return vec;
    }
    return nullptr;
}

void DataMan::SetBP3Deserializer(format::BP3Deserializer &bp3Deserializer) {}

void DataMan::SetIO(IO &io) {}

void DataMan::SetCallback(std::function<void(std::vector<char>)> callback) {}

void DataMan::ReadThread(std::shared_ptr<Transport> transport)
{

    // Initialize buffer
    std::vector<char> buffer(m_MaxReceiveBuffer);

    // Main loop
    while (m_Listening)
    {

        Transport::Status status;
        transport->IRead(buffer.data(), m_MaxReceiveBuffer, status);

        if (status.Bytes > 0)
        {

            std::shared_ptr<std::vector<char>> bufferQ =
                std::make_shared<std::vector<char>>(status.Bytes);

            std::memcpy(bufferQ->data(), buffer.data(), status.Bytes);

            PushBufferQueue(bufferQ);

            /*
                            // TODO: move to engine
                            if (GetBoolParameter(trans_params, "DumpFile"))
                            {
                                std::ofstream bpfile(stream_name,
            std::ios_base::binary);
                                bpfile.write(reinterpret_cast<const char
            *>(buffer->data()),
                                             status.Bytes);
                                bpfile.close();
                            }

                            m_BP3Deserializer->m_Data.Resize(
                                status.Bytes, "in DataMan Streaming Listener");
                            std::memcpy(m_BP3Deserializer->m_Data.m_Buffer.data(),
                                        buffer.data(), status.Bytes);
                            m_BP3Deserializer->ParseMetadata(m_BP3Deserializer->m_Data,
                                                             *m_IO);

                            const auto variablesInfo =
            m_IO->GetAvailableVariables();
                            for (const auto &variableInfoPair : variablesInfo)
                            {

                                std::string var = variableInfoPair.first;
                                std::string type = "null";

                                for (const auto &parameter :
            variableInfoPair.second)
                                {
                                    //  ** print out all parameters from BP
            metadata
                                        std::cout << "\tKey: " <<
            parameter.first
                                                  << "\t Value: " <<
            parameter.second <<
                                       "\n";
                                    if (parameter.first == "Type")
                                    {
                                        type = parameter.second;
                                    }
                                }

                                if (type == "compound")
                                {
                                    // not supported
                                }
            #define declare_type(T) \
                else if (type == GetType<T>()) \
                { \
                    adios2::Variable<T> *v = m_IO->InquireVariable<T>(var); \
                    m_BP3Deserializer->GetSyncVariableDataFromStream( \
                        *v, m_BP3Deserializer->m_Data); \
                    if (v->GetData() == nullptr) \
                    { \
                        throw("Data pointer obtained from BP deserializer is a
            nullptr");  \
                    } \
                    RunCallback(v->GetData(), "stream", var, type, v->m_Shape);
            \
                }
                                ADIOS2_FOREACH_TYPE_1ARG(declare_type)
            #undef declare_type
                            }
                                    */
        }
    }
}

bool DataMan::GetBoolParameter(const Params &params, std::string key)
{
    auto itKey = params.find(key);
    if (itKey != params.end())
    {
        std::string value = itKey->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        if (value == "yes" || value == "true")
        {
            return true;
        }
        else if (value == "no" || value == "false")
        {
            return false;
        }
    }
    return false;
}

} // end namespace transportman
} // end namespace adios2
