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
    SocketZmqReqRep(const MPI_Comm mpiComm, const int timeout,
                    const bool debugMode);
    virtual ~SocketZmqReqRep();
    void Open(const std::string &fullAddress, const Mode openMode) final;
    void Open(const std::string &ipAddress, const std::string &port,
              const std::string &name, const Mode openMode) final;
    void SetBuffer(char *buffer, size_t size) final;
    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;
    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;
    void IWrite(const char *buffer, size_t size, Status &status,
                size_t start = MaxSizeT) final;
    void IRead(char *buffer, size_t size, Status &status,
               size_t start = MaxSizeT) final;
    void Flush() final;
    void Close() final;

private:
    static void *m_Context;
    void *m_Socket = nullptr;
    const int m_Timeout;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_SOCKET_SOCKETZMQREQREP_H_ */
