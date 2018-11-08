/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmqPubSub.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "SocketZmqPubSub.h"

#include <iostream>
#include <zmq.h>

namespace adios2
{
namespace transport
{

SocketZmqPubSub::SocketZmqPubSub(const MPI_Comm mpiComm, const bool debugMode)
: SocketZmq("wan", "zmqpubsub", mpiComm, debugMode)
{
    m_Context = zmq_ctx_new();
    if (m_Context == nullptr || m_Context == NULL)
    {
        throw std::runtime_error("[SocketZmqPubSub::SocketZmqPubSub] Creating "
                                 "ZeroMQ context failed.");
    }
}

SocketZmqPubSub::~SocketZmqPubSub()
{
    if (m_Socket)
    {
        zmq_close(m_Socket);
    }
    if (m_Context)
    {
        zmq_ctx_destroy(m_Context);
    }
}

void SocketZmqPubSub::Open(const std::string &ipAddress,
                           const std::string &port, const std::string &name,
                           const Mode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;
    const std::string fullIP("tcp://" + ipAddress + ":" + port);
    std::string openModeStr;

    int error = -1;
    ProfilerStart("open");
    if (m_OpenMode == Mode::Write)
    {
        openModeStr = "Write";
        m_Socket = zmq_socket(m_Context, ZMQ_PUB);
        error = zmq_bind(m_Socket, fullIP.c_str());
    }
    else if (m_OpenMode == Mode::Read)
    {
        openModeStr = "Read";
        m_Socket = zmq_socket(m_Context, ZMQ_SUB);
        error = zmq_connect(m_Socket, fullIP.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SUBSCRIBE, "", 0);
    }
    else
    {
        throw std::invalid_argument(
            "[SocketZmqPubSub::Open] received invalid OpenMode parameter");
    }
    ProfilerStop("open");

    if (m_DebugMode)
    {
        std::cout << "[SocketZmq Transport] ";
        std::cout << "OpenMode: " << openModeStr << ", ";
        std::cout << "WorkflowMode: subscribe, ";
        std::cout << "IPAddress: " << ipAddress << ", ";
        std::cout << "Port: " << port << ", ";
        std::cout << std::endl;
    }

    if (error)
    {
        throw std::runtime_error(
            "[SocketZmqPubSub::Open] zmq_connect() failed with " +
            std::to_string(error));
    }

    if (m_Socket == nullptr || m_Socket == NULL)
    {
        throw std::ios_base::failure(
            "[SocketZmqPubSub::Open] couldn't open socket for address " +
            m_Name);
    }

    m_IsOpen = true;
}

void SocketZmqPubSub::Open(const std::string &name, const Mode openMode)
{
    Open("127.0.0.1", "12306", name, openMode);
}

void SocketZmqPubSub::SetBuffer(char *buffer, size_t size) {}

void SocketZmqPubSub::Write(const char *buffer, size_t size, size_t start) {}

void SocketZmqPubSub::Read(char *buffer, size_t size, size_t start) {}

void SocketZmqPubSub::IWrite(const char *buffer, size_t size, Status &status,
                             size_t start)
{
    int retInt = 0;
    ProfilerStart("write");
    retInt = zmq_send(m_Socket, buffer, size, ZMQ_DONTWAIT);
    ProfilerStop("write");
    if (retInt < 0)
    {
        throw std::ios_base::failure(
            "[SocketZmqPubSub::IWrite] couldn't send message " + m_Name);
    }
}

void SocketZmqPubSub::IRead(char *buffer, size_t size, Status &status,
                            size_t start)
{
    ProfilerStart("read");
    int bytes = zmq_recv(m_Socket, buffer, size, ZMQ_DONTWAIT);
    ProfilerStop("read");
    if (bytes > 0)
    {
        status.Bytes = bytes;
        status.Running = true;
    }
    else
    {
        status.Bytes = 0;
        status.Running = true;
    }
    if (bytes == size)
    {
        status.Successful = true;
    }
    else
    {
        status.Successful = false;
    }
}

void SocketZmqPubSub::Flush() {}

void SocketZmqPubSub::Close()
{
    if (m_Socket != nullptr)
    {
        zmq_close(m_Socket);
    }
}

} // end namespace transport
} // end namespace adios2
