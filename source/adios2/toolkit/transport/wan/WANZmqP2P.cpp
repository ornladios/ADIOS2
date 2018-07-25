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

WANZmqP2P::WANZmqP2P(const std::string ipAddress, const std::string port,
                     MPI_Comm mpiComm, const std::string transportMode,
                     const bool debugMode)
: Transport("wan", "zmq", mpiComm, debugMode), m_IPAddress(ipAddress),
  m_Port(port), m_WorkflowMode(transportMode)
{
    m_Context = zmq_ctx_new();
    if (m_Context == nullptr || m_Context == NULL)
    {
        throw std::runtime_error("ERROR: Creating ZeroMQ context failed");
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

int WANZmqP2P::OpenSubscribe(const std::string &name, const Mode openMode,
                             const std::string ip)
{
    // PubSub mode uses the ZeroMQ Pub/Sub scheme and requires the IP address
    // and port of the publisher (sender). Multiple subscriptors are allowed to
    // subscribe to the same publisher.

    int error = -1;

    if (m_OpenMode == Mode::Write)
    {
        m_OpenModeStr = "Write";
        m_Socket = zmq_socket(m_Context, ZMQ_PUB);
        error = zmq_bind(m_Socket, ip.c_str());
    }
    else if (m_OpenMode == Mode::Read)
    {
        m_OpenModeStr = "Read";
        m_Socket = zmq_socket(m_Context, ZMQ_SUB);
        error = zmq_connect(m_Socket, ip.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SUBSCRIBE, "", 0);
    }
    else
    {
        throw std::invalid_argument(
            "WANZmqP2P::Open received invalid OpenMode parameter");
    }

    if (m_DebugMode)
    {
        std::cout << "[WANZmqP2P Transport] OpenMode: " << m_OpenModeStr;
        std::cout << ", Address: " << ip << std::endl;
    }

    return error;
}

int WANZmqP2P::OpenPush(const std::string &name, const Mode openMode,
                        const std::string ip)
{
    int error = -1;

    if (m_OpenMode == Mode::Write)
    {
        m_Socket = zmq_socket(m_Context, ZMQ_REQ);
        error = zmq_connect(m_Socket, ip.c_str());
    }

    else if (m_OpenMode == Mode::Read)
    {
        m_Socket = zmq_socket(m_Context, ZMQ_REP);
        error = zmq_bind(m_Socket, ip.c_str());
    }
    else
    {
        throw std::invalid_argument(
            "WANZmqP2P::Open received invalid OpenMode parameter");
    }

    return error;
}

int WANZmqP2P::OpenQuery(const std::string &name, const Mode openMode,
                         const std::string ip)
{
    return 0;
}

void WANZmqP2P::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;
    const std::string fullIP("tcp://" + m_IPAddress + ":" + m_Port);

    ProfilerStart("open");

    int err;
    if (m_WorkflowMode == "subscribe")
    {
        err = OpenSubscribe(name, openMode, fullIP);
    }
    else if (m_WorkflowMode == "push")
    {
        err = OpenPush(name, openMode, fullIP);
    }
    else if (m_WorkflowMode == "query")
    {
        err = OpenQuery(name, openMode, fullIP);
    }
    else
    {
        throw std::invalid_argument(
            "WANZmqP2P::Open received wrong WorkflowMode parameter: " +
            m_WorkflowMode + ". Should be subscribe, push or query");
    }

    if (err)
    {
        throw std::runtime_error("ERROR: zmq_connect() failed with " +
                                 std::to_string(err));
    }

    ProfilerStop("open");

    if (m_DebugMode)
    {
        if (m_Socket == nullptr || m_Socket == NULL) // something goes wrong
        {
            throw std::ios_base::failure(
                "ERROR: couldn't open socket for address " + m_Name +
                ", in call to WANZmqP2P Open\n");
        }
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
    std::string retString = "OK";
    if (m_WorkflowMode == "subscribe")
    {
        ProfilerStart("write");
        retInt = zmq_send(m_Socket, buffer, size, 0);
        ProfilerStop("write");
    }
    else if (m_WorkflowMode == "push")
    {
    }
    else if (m_WorkflowMode == "query")
    {
    }
    else
    {
        throw std::runtime_error(
            "Unknown WorkflowMode caught in WANZmqP2P::IWrite");
    }

    if (retInt < 0 || retString != "OK")
    {
        throw std::ios_base::failure("ERROR: couldn't send message " + m_Name +
                                     ", in call to WANZmqP2P::IWrite\n");
    }
}

void WANZmqP2P::IRead(char *buffer, size_t size, Status &status, size_t start)
{
    if (m_WorkflowMode == "subscribe")
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
    else if (m_WorkflowMode == "push")
    {
    }
    else if (m_WorkflowMode == "query")
    {
    }
    else
    {
        throw std::runtime_error(
            "Unknown WorkflowMode caught in WANZmqP2P::IRead");
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
