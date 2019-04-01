/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmqReqRep.h
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQREQREP_H_
#define ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQREQREP_H_

#include "adios2/toolkit/transport/socket/SocketZmq.h"

namespace adios2
{
namespace transport
{

class SocketZmqReqRep : public SocketZmq
{

public:
    SocketZmqReqRep(const int timeout);
    virtual ~SocketZmqReqRep();
    int Open(const std::string &fullAddress, const Mode openMode) final;
    int Write(const char *buffer, size_t size) final;
    int Read(char *buffer, size_t size) final;
    int Close() final;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQREQREP_H_ */
