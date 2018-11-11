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

class SocketZmq : public Transport
{
public:
    SocketZmq(const std::string type, const std::string library,
              const MPI_Comm mpiComm, const bool debugMode)
    : Transport("wan", "zmq", mpiComm, debugMode)
    {
    }
    virtual ~SocketZmq() = default;
    virtual void Open(const std::string &ipAddress, const std::string &port,
                      const std::string &name, const Mode openMode) = 0;

private:
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_ */
