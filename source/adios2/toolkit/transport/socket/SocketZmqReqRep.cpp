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

SocketZmqReqRep::SocketZmqReqRep(const MPI_Comm mpiComm, const int timeout)
: m_Timeout(timeout)
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


void SocketZmqReqRep::Open(const std::string &fullAddress, const Mode openMode)
{
    std::string openModeStr;
    int error = -1;
    if (openMode == Mode::Write)
    {
        openModeStr = "Write/ZMQ_REP";
        m_Socket = zmq_socket(m_Context, ZMQ_REP);
        error = zmq_bind(m_Socket, fullAddress.c_str());
        zmq_setsockopt(m_Socket, ZMQ_RCVTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else if (openMode == Mode::Read)
    {
        openModeStr = "Read/ZMQ_REQ";
        m_Socket = zmq_socket(m_Context, ZMQ_REQ);
        error = zmq_connect(m_Socket, fullAddress.c_str());
        zmq_setsockopt(m_Socket, ZMQ_SNDTIMEO, &m_Timeout, sizeof(m_Timeout));
    }
    else
    {
        throw std::invalid_argument(
            "[SocketZmqReqRep::Open] invalid OpenMode parameter");
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "[SocketZmqReqRep Transport] ";
        std::cout << "OpenMode: " << openModeStr << ", ";
        std::cout << "Address: " << fullAddress << ", ";
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
}


int SocketZmqReqRep::Write(const char *buffer, size_t size)
{
    return zmq_send(m_Socket, buffer, size, 0);
}


int SocketZmqReqRep::Read(char *buffer, size_t size)
{
    return zmq_recv(m_Socket, buffer, size, 0);
}


void SocketZmqReqRep::Close()
{
    if (m_Socket)
    {
        zmq_close(m_Socket);
    }
}

} // end namespace transport
} // end namespace adios2
