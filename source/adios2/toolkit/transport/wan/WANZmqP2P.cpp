/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WANZmqP2P.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "WANZmqP2P.h"

#include <iostream>
#include <zmq.h>

namespace adios2
{
namespace transport
{

WANZmqP2P::WANZmqP2P(const std::string &ipAddress, const std::string &port,
                     const MPI_Comm mpiComm, const int timeout,
                     const bool debugMode)
: Transport("wan", "zmq", mpiComm, debugMode), m_IPAddress(ipAddress),
  m_Port(port), m_Timeout(timeout)
{
    m_Context = zmq_ctx_new();
    if (m_Context == nullptr || m_Context == NULL)
    {
        throw std::runtime_error(
            "[WANZmqP2P::WANZmqP2P] Creating ZeroMQ context failed.");
    }
}

WANZmqP2P::~WANZmqP2P()
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

void WANZmqP2P::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;
    const std::string fullIP("tcp://" + m_IPAddress + ":" + m_Port);

    int error = -1;
    ProfilerStart("open");
    if (m_OpenMode == Mode::Write)
    {
        m_OpenModeStr = "Write";
        m_Socket = zmq_socket(m_Context, ZMQ_REQ);
        error = zmq_connect(m_Socket, fullIP.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SNDTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else if (m_OpenMode == Mode::Read)
    {
        m_OpenModeStr = "Read";
        m_Socket = zmq_socket(m_Context, ZMQ_REP);
        error = zmq_bind(m_Socket, fullIP.c_str());
        zmq_setsockopt(m_Socket, ZMQ_RCVTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else
    {
        throw std::invalid_argument(
            "[WANZmqP2P::Open] invalid OpenMode parameter");
    }
    ProfilerStop("open");

    if (m_DebugMode)
    {
        std::cout << "[WANZmq Transport] ";
        std::cout << "OpenMode: " << m_OpenModeStr << ", ";
        std::cout << "WorkflowMode: p2p, ";
        std::cout << "IPAddress: " << m_IPAddress << ", ";
        std::cout << "Port: " << m_Port << ", ";
        std::cout << "Timeout: " << m_Timeout << ", ";
        std::cout << std::endl;
    }

    if (error)
    {
        throw std::runtime_error(
            "[WANZmqP2P::Open] zmq_connect() failed with " +
            std::to_string(error));
    }

    if (m_Socket == nullptr || m_Socket == NULL)
    {
        throw std::ios_base::failure(
            "[WANZmqP2P::Open] couldn't open socket for address " + fullIP);
    }
    m_IsOpen = true;
}

void WANZmqP2P::SetBuffer(char *buffer, size_t size) {}

void WANZmqP2P::Write(const char *buffer, size_t size, size_t start) {}

void WANZmqP2P::Read(char *buffer, size_t size, size_t start) {}

void WANZmqP2P::IWrite(const char *buffer, size_t size, Status &status,
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
            "[WANZmqP2P::IWrite] couldn't send message " + m_Name);
    }
}

void WANZmqP2P::IRead(char *buffer, size_t size, Status &status, size_t start)
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

void WANZmqP2P::Flush() {}

void WANZmqP2P::Close()
{
    if (m_Socket != nullptr)
    {
        zmq_close(m_Socket);
    }
}

} // end namespace transport
} // end namespace adios2
