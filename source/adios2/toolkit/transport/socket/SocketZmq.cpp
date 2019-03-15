/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmq.cpp
 *
 *  Created on: Feb 2, 2019
 *      Author: Jason Wang wangr1@ornl.gov
 */

#include "SocketZmq.h"

#include <iostream>
#include <zmq.h>

namespace adios2
{
namespace transport
{

SocketZmq::SocketZmq(const int timeout)
{
    m_Context = zmq_ctx_new();
    if (!m_Context)
    {
        throw std::runtime_error("Creating ZeroMQ context failed.");
    }
}

SocketZmq::~SocketZmq()
{
    if (m_Context)
    {
        zmq_ctx_destroy(m_Context);
    }
}

} // end namespace transport
} // end namespace adios2
