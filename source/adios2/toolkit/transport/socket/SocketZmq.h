/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmqP2P.h
 *
 *  Created on: Nov 11, 2018
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_
#define ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_

#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

class SocketZmq
{
public:
    SocketZmq(const int timeout);
    virtual ~SocketZmq();
    virtual int Open(const std::string &address, const Mode openMode) = 0;
    virtual int Write(const char *buffer, const size_t size) = 0;
    virtual int Read(char *buffer, const size_t size) = 0;
    virtual int Close() = 0;

protected:
    void *m_Context = nullptr;
    void *m_Socket = nullptr;
    const int m_Timeout = 3;
    int m_Verbosity = 0;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_ */
