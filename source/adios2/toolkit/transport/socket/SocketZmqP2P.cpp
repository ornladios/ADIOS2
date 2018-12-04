/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmqP2P.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "SocketZmqP2P.h"

#include <iostream>
#include <zmq.h>

namespace adios2
{
namespace transport
{

void *SocketZmqP2P::m_Context = nullptr;

SocketZmqP2P::SocketZmqP2P(const MPI_Comm mpiComm, const int timeout,
                           const bool debugMode)
: SocketZmq("wan", "zmqp2p", mpiComm, debugMode), m_Timeout(timeout)
{
    m_Context = zmq_ctx_new();
    if (m_Context == nullptr || m_Context == NULL)
    {
        throw std::runtime_error(
            "[SocketZmqP2P::SocketZmqP2P] Creating ZeroMQ context failed.");
    }
}

SocketZmqP2P::~SocketZmqP2P()
{
    if (m_Socket)
    {
        zmq_close(m_Socket);
    }
}

void SocketZmqP2P::Open(const std::string &name, const Mode openMode)
{

    Open("127.0.0.1", "12306", name, openMode);
}

void SocketZmqP2P::Open(const std::string &ipAddress, const std::string &port,
                        const std::string &name, const Mode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;
    const std::string fullIP("tcp://" + ipAddress + ":" + port + "\0");
    std::string openModeStr;

    int error = -1;
    ProfilerStart("open");
    if (m_OpenMode == Mode::Write)
    {
        openModeStr = "Write";
        m_Socket = zmq_socket(m_Context, ZMQ_REQ);
        error = zmq_connect(m_Socket, fullIP.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SNDTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else if (m_OpenMode == Mode::Read)
    {
        openModeStr = "Read";
        m_Socket = zmq_socket(m_Context, ZMQ_REP);
        error = zmq_bind(m_Socket, fullIP.c_str());
        zmq_setsockopt(m_Socket, ZMQ_RCVTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else
    {
        throw std::invalid_argument(
            "[SocketZmqP2P::Open] invalid OpenMode parameter");
    }
    ProfilerStop("open");

    if (m_DebugMode)
    {
        std::cout << "[SocketZmq Transport] ";
        std::cout << "OpenMode: " << openModeStr << ", ";
        std::cout << "WorkflowMode: p2p, ";
        std::cout << "IPAddress: " << ipAddress << ", ";
        std::cout << "Port: " << port << ", ";
        std::cout << "Timeout: " << m_Timeout << ", ";
        std::cout << std::endl;
    }

    if (error)
    {
        throw std::runtime_error(
            "[SocketZmqP2P::Open] zmq_connect() failed with " +
            std::to_string(error));
    }

    if (m_Socket == nullptr || m_Socket == NULL)
    {
        throw std::ios_base::failure(
            "[SocketZmqP2P::Open] couldn't open socket for address " + fullIP);
    }
    m_IsOpen = true;
}

void SocketZmqP2P::SetBuffer(char *buffer, size_t size) {}

void SocketZmqP2P::Write(const char *buffer, size_t size, size_t start) {}

void SocketZmqP2P::Read(char *buffer, size_t size, size_t start) {}

void SocketZmqP2P::IWrite(const char *buffer, size_t size, Status &status,
                          size_t start)
{
    int retInt = 0;
    char retChar[10];
    ProfilerStart("write");
    retInt = zmq_send(m_Socket, buffer, size, 0);
    zmq_recv(m_Socket, retChar, 4, 0);
    ProfilerStop("write");
    const std::string retString = retChar;
    if (retInt < 0 || retString != "OK")
    {
        throw std::ios_base::failure(
            "[SocketZmqP2P::IWrite] couldn't send message " + m_Name);
    }
}

void SocketZmqP2P::IRead(char *buffer, size_t size, Status &status,
                         size_t start)
{
    const std::string reply = "OK";
    ProfilerStart("read");
    int bytes = zmq_recv(m_Socket, buffer, size, 0);
    zmq_send(m_Socket, reply.c_str(), 4, 0);
    ProfilerStop("read");
    if (bytes > 0)
    {
        status.Bytes = bytes;
        status.Running = true;
        status.Successful = true;
    }
    else
    {
        status.Bytes = 0;
        status.Running = true;
        status.Successful = false;
    }
}

void SocketZmqP2P::Flush() {}

void SocketZmqP2P::Close()
{
    if (m_Socket)
    {
        zmq_close(m_Socket);
    }
}

} // end namespace transport
} // end namespace adios2
