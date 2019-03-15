/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SocketZmqPubSub.h
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQPUBSUB_H_
#define ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQPUBSUB_H_

#include "adios2/toolkit/transport/socket/SocketZmq.h"

namespace adios2
{
namespace transport
{

class SocketZmqPubSub : public SocketZmq
{

public:
    SocketZmqPubSub(const int timeout);
    virtual ~SocketZmqPubSub();
    int Open(const std::string &address, const Mode openMode) final;
    int Write(const char *buffer, const size_t size) final;
    int Read(char *buffer, const size_t size) final;
    int Close() final;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQPUBSUB_H_ */
