/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmqReqRep.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "SocketZmqReqRep.h"

#include <iostream>
#include <zmq.h>

namespace adios2
{
namespace transport
{

void *SocketZmqReqRep::m_Context = nullptr;

SocketZmqReqRep::SocketZmqReqRep(const MPI_Comm mpiComm, const int timeout,
                                 const bool debugMode)
: SocketZmq("socket", "zmqreqrep", mpiComm, debugMode), m_Timeout(timeout)
{
    if (m_Context == nullptr || m_Context == NULL)
    {
        m_Context = zmq_ctx_new();
    }
    if (m_Context == nullptr || m_Context == NULL)
    {
        throw std::runtime_error("[SocketZmqReqRep::SocketZmqReqRep] Creating "
                                 "ZeroMQ context failed.");
    }
}

SocketZmqReqRep::~SocketZmqReqRep()
{
    if (m_Socket)
    {
        zmq_close(m_Socket);
    }
}

void SocketZmqReqRep::Open(const std::string &name, const Mode openMode)
{
    int port = 12306 + m_RankMPI;
    if (openMode == Mode::Write)
    {
        Open("0.0.0.0", std::to_string(port), name, openMode);
    }
    else if (openMode == Mode::Read)
    {
        Open("127.0.0.1", std::to_string(port), name, openMode);
    }
    else
    {
        throw std::invalid_argument(
            "[SocketZmqReqRep::Open] invalid OpenMode parameter");
    }
}

void SocketZmqReqRep::Open(const std::string &ipAddress,
                           const std::string &port, const std::string &name,
                           const Mode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;
    const std::string fullAddress("tcp://" + ipAddress + ":" + port);
    std::string openModeStr;

    int error = -1;
    if (m_OpenMode == Mode::Write)
    {
        openModeStr = "Write";
        m_Socket = zmq_socket(m_Context, ZMQ_REP);
        error = zmq_bind(m_Socket, fullAddress.c_str());
        zmq_setsockopt(m_Socket, ZMQ_RCVTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else if (m_OpenMode == Mode::Read)
    {
        openModeStr = "Read";
        m_Socket = zmq_socket(m_Context, ZMQ_REQ);
        error = zmq_connect(m_Socket, fullAddress.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SNDTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else
    {
        throw std::invalid_argument(
            "[SocketZmqReqRep::Open] invalid OpenMode parameter");
    }

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
            "[SocketZmqReqRep::Open] zmq_connect() failed with " +
            std::to_string(error));
    }

    if (m_Socket == nullptr || m_Socket == NULL)
    {
        throw std::ios_base::failure(
            "[SocketZmqReqRep::Open] couldn't open socket for address " +
            fullAddress);
    }
    m_IsOpen = true;
}

void SocketZmqReqRep::SetBuffer(char *buffer, size_t size) {}

void SocketZmqReqRep::Write(const char *buffer, size_t size, size_t start) {}

void SocketZmqReqRep::Read(char *buffer, size_t size, size_t start) {}

void SocketZmqReqRep::IWrite(const char *buffer, size_t size, Status &status,
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
            "[SocketZmqReqRep::IWrite] couldn't send message " + m_Name);
    }
}

void SocketZmqReqRep::IRead(char *buffer, size_t size, Status &status,
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

void SocketZmqReqRep::Flush() {}

void SocketZmqReqRep::Close()
{
    if (m_Socket)
    {
        zmq_close(m_Socket);
    }
}

} // end namespace transport
} // end namespace adios2
