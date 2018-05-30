/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WANZmq.cpp
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "WANZmq.h"

#include <iostream>
#include <zmq.h>

namespace adios2
{
namespace transport
{

WANZmq::WANZmq(const std::string ipAddress, const std::string port,
               MPI_Comm mpiComm, const std::string transportMode,
               const bool debugMode)
: Transport("wan", "zmq", mpiComm, debugMode), m_IPAddress(ipAddress),
  m_Port(port), m_TransportMode(transportMode)
{
    m_Context = zmq_ctx_new();
    if (m_Context == nullptr || m_Context == NULL)
    {
        throw std::runtime_error("ERROR: Creating ZeroMQ context failed");
    }
    if (m_DebugMode)
    {
        std::cout << "[WANZmq] IP Address " << ipAddress << std::endl;
        std::cout << "[WANZmq] Port " << port << std::endl;
    }
}

WANZmq::~WANZmq()
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

int WANZmq::OpenSubscribe(const std::string &name, const Mode openMode,
                          const std::string ip)
{
    // PubSub mode uses the ZeroMQ Pub/Sub scheme and requires the IP address
    // and port of the publisher (sender). Multiple subscriptors are allowed to
    // subscribe to the same publisher.

    int error = -1;

    if (m_OpenMode == Mode::Write)
    {
        m_Socket = zmq_socket(m_Context, ZMQ_PUB);
        error = zmq_bind(m_Socket, ip.c_str());
    }
    else if (m_OpenMode == Mode::Read)
    {
        m_Socket = zmq_socket(m_Context, ZMQ_SUB);
        error = zmq_connect(m_Socket, ip.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SUBSCRIBE, "", 0);
    }

    return error;
}

int WANZmq::OpenPush(const std::string &name, const Mode openMode,
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

    return error;
}

int WANZmq::OpenQuery(const std::string &name, const Mode openMode,
                      const std::string ip)
{
    return 0;
}

void WANZmq::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;
    const std::string fullIP("tcp://" + m_IPAddress + ":" + m_Port);

    ProfilerStart("open");

    int err;
    if (m_TransportMode == "subscribe")
    {
        err = OpenSubscribe(name, openMode, fullIP);
    }
    else if (m_TransportMode == "push")
    {
        err = OpenPush(name, openMode, fullIP);
    }
    else if (m_TransportMode == "query")
    {
        err = OpenQuery(name, openMode, fullIP);
    }
    else
    {
        throw std::runtime_error(
            "WANZmq::Open received wrong WorkflowMode parameter" +
            m_TransportMode + ". Should be subscribe, push or query");
    }

    if (err)
    {
        throw std::runtime_error("ERROR: zmq_connect() failed with " +
                                 std::to_string(err));
    }

    ProfilerStop("open");

    if (m_DebugMode)
    {
        std::cout << "[WANZmq] Open " << std::endl;
    }

    if (m_DebugMode)
    {
        if (m_Socket == nullptr || m_Socket == NULL) // something goes wrong
        {
            throw std::ios_base::failure(
                "ERROR: couldn't open socket for address " + m_Name +
                ", in call to WANZmq Open\n");
        }
    }
    m_IsOpen = true;
}

void WANZmq::SetBuffer(char *buffer, size_t size) {}

void WANZmq::Write(const char *buffer, size_t size, size_t start) {}

void WANZmq::Read(char *buffer, size_t size, size_t start) {}

void WANZmq::IWrite(const char *buffer, size_t size, Status &status,
                    size_t start)
{
    int retInt = 0;
    std::string retString = "OK";
    if (m_TransportMode == "subscribe")
    {
        ProfilerStart("write");
        retInt = zmq_send(m_Socket, buffer, size, 0);
        ProfilerStop("write");
    }
    else if (m_TransportMode == "push")
    {
    }
    else if (m_TransportMode == "query")
    {
    }
    else
    {
        throw std::runtime_error(
            "Unknown WorkflowMode caught in WANZmq::IWrite");
    }

    if (retInt < 0 || retString != "OK")
    {
        throw std::ios_base::failure("ERROR: couldn't send message " + m_Name +
                                     ", in call to WANZmq::IWrite\n");
    }
}

void WANZmq::IRead(char *buffer, size_t size, Status &status, size_t start)
{
    if (m_TransportMode == "subscribe")
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
    else if (m_TransportMode == "push")
    {
    }
    else if (m_TransportMode == "query")
    {
    }
    else
    {
        throw std::runtime_error(
            "Unknown WorkflowMode caught in WANZmq::IRead");
    }
}

void WANZmq::Flush() {}

void WANZmq::Close()
{
    if (m_Socket != nullptr)
    {
        zmq_close(m_Socket);
    }
}

} // end namespace transport
} // end namespace adios2
