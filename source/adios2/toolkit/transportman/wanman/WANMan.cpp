/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WANMan.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include <fstream>  //TODO go away
#include <iostream> //TODO go away

#include "WANMan.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_ZEROMQ
#include "adios2/toolkit/transport/socket/SocketZmqP2P.h"
#include "adios2/toolkit/transport/socket/SocketZmqPubSub.h"
#endif

namespace adios2
{
namespace transportman
{

WANMan::WANMan(MPI_Comm mpiComm, const bool debugMode)
: m_MpiComm(mpiComm), m_DebugMode(debugMode)
{
}

WANMan::~WANMan()
{
    auto start_time = std::chrono::system_clock::now();
    while (true)
    {
        int s = 0;
        m_Mutex.lock();
        for (const auto &i : m_BufferQueue)
        {
            if (i.size() != 0)
            {
                ++s;
            }
        }
        m_Mutex.unlock();
        if (s == 0)
        {
            break;
        }
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        if (duration.count() > m_Timeout)
        {
            break;
        }
        // add a sleep here because this loop is occupying the buffer queue
        // lock, and this sleep would make time for reader or writer thread and
        // help it finish sooner.
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    for (auto &readThread : m_ReadThreads)
    {
        m_Reading = false;
        readThread.join();
    }
    for (auto &writeThread : m_WriteThreads)
    {
        m_Writing = false;
        writeThread.join();
    }
}

void WANMan::SetMaxReceiveBuffer(size_t size) { m_MaxReceiveBuffer = size; }

void WANMan::OpenSocketTransports(const std::vector<std::string> &streamNames,
                                  const std::vector<Params> &paramsVector,
                                  const Mode mode,
                                  const std::string workflowMode,
                                  const bool profile)
{
    m_TransportsParameters = paramsVector;
    m_BufferQueue.resize(streamNames.size());

    for (size_t i = 0; i < streamNames.size(); ++i)
    {
        // Get parameters
        std::string library;
        GetStringParameter(paramsVector[i], "Library", library);
        std::string ip;
        GetStringParameter(paramsVector[i], "IPAddress", ip);
        std::string port;
        GetStringParameter(paramsVector[i], "Port", port);
        GetIntParameter(paramsVector[i], "Timeout", m_Timeout);

        // Calculate port number
        int mpiRank, mpiSize;
        MPI_Comm_rank(m_MpiComm, &mpiRank);
        MPI_Comm_size(m_MpiComm, &mpiSize);
        if (port.empty())
        {
            port = std::to_string(stoi(port) + i * mpiSize);
        }
        port = std::to_string(stoi(port) + mpiRank);

        if (library == "zmq" || library == "ZMQ")
        {
#ifdef ADIOS2_HAVE_ZEROMQ
            std::shared_ptr<transport::SocketZmq> wanTransport;
            if (workflowMode == "subscribe")
            {
                wanTransport = std::make_shared<transport::SocketZmqPubSub>(
                    m_MpiComm, m_DebugMode);
            }
            else if (workflowMode == "p2p")
            {
                wanTransport = std::make_shared<transport::SocketZmqP2P>(
                    m_MpiComm, m_Timeout, m_DebugMode);
            }
            else if (workflowMode == "query")
            {
            }
            else
            {
                throw(std::invalid_argument(
                    "[WANMan::OpenSocketTransports] workflow mode " +
                    workflowMode + " not supported."));
            }

            wanTransport->Open(ip, port, streamNames[i], mode);
            m_Transports.emplace(i, wanTransport);

            // launch thread
            if (mode == Mode::Read)
            {
                m_Reading = true;
                m_ReadThreads.emplace_back(
                    std::thread(&WANMan::ReadThread, this, wanTransport));
            }
            else if (mode == Mode::Write)
            {
                m_Writing = true;
                m_WriteThreads.emplace_back(
                    std::thread(&WANMan::WriteThread, this, wanTransport, i));
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

void WANMan::WriteSocket(std::shared_ptr<std::vector<char>> buffer, size_t id)
{
    PushBufferQueue(buffer, id);
}

void WANMan::WriteSocket(const std::vector<char> &buffer, size_t transportId)
{
    if (transportId >= m_Transports.size())
    {
        throw std::runtime_error(
            "ERROR: No valid transports found, from WANMan::WriteSocket()");
    }

    m_Transports[transportId]->Write(buffer.data(), buffer.size());
}

std::shared_ptr<std::vector<char>> WANMan::ReadSocket(size_t id)
{
    return PopBufferQueue(id);
}

void WANMan::PushBufferQueue(std::shared_ptr<std::vector<char>> v, size_t id)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    m_BufferQueue[id].push(v);
}

std::shared_ptr<std::vector<char>> WANMan::PopBufferQueue(size_t id)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    if (m_BufferQueue[id].size() > 0)
    {
        std::shared_ptr<std::vector<char>> vec = m_BufferQueue[id].front();
        m_BufferQueue[id].pop();
        return vec;
    }
    return nullptr;
}

void WANMan::WriteThread(std::shared_ptr<Transport> transport, size_t id)
{
    while (m_Writing)
    {
        Transport::Status status;
        std::shared_ptr<std::vector<char>> buffer = PopBufferQueue(id);
        if (buffer != nullptr)
        {
            if (buffer->size() > 0)
            {
                transport->IWrite(buffer->data(), buffer->size(), status);
            }
        }
    }
}

void WANMan::ReadThread(std::shared_ptr<Transport> transport)
{
    std::vector<char> buffer(m_MaxReceiveBuffer);
    while (m_Reading)
    {
        Transport::Status status;
        transport->IRead(buffer.data(), m_MaxReceiveBuffer, status);
        if (status.Bytes > 0)
        {
            std::shared_ptr<std::vector<char>> bufferQ =
                std::make_shared<std::vector<char>>(status.Bytes);
            std::memcpy(bufferQ->data(), buffer.data(), status.Bytes);
            PushBufferQueue(bufferQ, 0);
        }
    }
}

bool WANMan::GetBoolParameter(const Params &params, const std::string key)
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

bool WANMan::GetStringParameter(const Params &params, const std::string key,
                                std::string &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = it->second;
        return true;
    }
    return false;
}

bool WANMan::GetIntParameter(const Params &params, const std::string key,
                             int &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        try
        {
            value = std::stoi(it->second);
            return true;
        }
        catch (std::exception &e)
        {
            std::cout << "Parameter " << key
                      << " should be an integer in string format. However, "
                      << e.what() << " has been caught while trying to convert "
                                     "the value to an integer."
                      << std::endl;
            return false;
        }
    }
    return false;
}

} // end namespace transportman
} // end namespace adios2
