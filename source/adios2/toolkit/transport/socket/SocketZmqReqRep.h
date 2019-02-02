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

class SocketZmqReqRep
{

public:
    SocketZmqReqRep(const MPI_Comm mpiComm, const int timeout);
    virtual ~SocketZmqReqRep();
    int Open(const std::string &fullAddress, const Mode openMode);
    int Write(const char *buffer, size_t size);
    int Read(char *buffer, size_t size);
    void Close();

private:
    void *m_Context = nullptr;
    void *m_Socket = nullptr;
    const int m_Timeout;
    int m_Verbosity = 0;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQREQREP_H_ */
