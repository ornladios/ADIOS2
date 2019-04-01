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

SocketZmqPubSub::SocketZmqPubSub(const int timeout) : SocketZmq(timeout) {}

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

int SocketZmqPubSub::Open(const std::string &address, const Mode openMode)
{
    std::string openModeStr;

    int error = -1;
    if (openMode == Mode::Write)
    {
        openModeStr = "Write";
        m_Socket = zmq_socket(m_Context, ZMQ_PUB);
        error = zmq_bind(m_Socket, address.c_str());
    }
    else if (openMode == Mode::Read)
    {
        openModeStr = "Read";
        m_Socket = zmq_socket(m_Context, ZMQ_SUB);
        error = zmq_connect(m_Socket, address.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SUBSCRIBE, "", 0);
    }
    else
    {
        throw std::invalid_argument(
            "[SocketZmqPubSub::Open] received invalid OpenMode parameter");
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "[SocketZmqPubSub Transport] ";
        std::cout << "OpenMode: " << openModeStr << ", ";
        std::cout << "Address: " << address << ", ";
        std::cout << std::endl;
    }

    return error;
}

int SocketZmqPubSub::Write(const char *buffer, const size_t size)
{
    return zmq_send(m_Socket, buffer, size, ZMQ_DONTWAIT);
}

int SocketZmqPubSub::Read(char *buffer, const size_t size)
{
    return zmq_recv(m_Socket, buffer, size, ZMQ_DONTWAIT);
}

int SocketZmqPubSub::Close()
{
    if (m_Socket == nullptr)
    {
        return -1;
    }
    else
    {
        zmq_close(m_Socket);
    }
    return 0;
}

} // end namespace transport
} // end namespace adios2
