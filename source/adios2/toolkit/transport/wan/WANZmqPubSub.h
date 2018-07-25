/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WANZmqPubSub.h
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQPUBSUB_H_
#define ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQPUBSUB_H_

#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

class WANZmqPubSub : public Transport
{

public:
    WANZmqPubSub(const std::string &ipAddress, const std::string &port,
                 const MPI_Comm mpiComm, const bool debugMode);
    ~WANZmqPubSub();

    void Open(const std::string &name, const Mode openMode) final;

    void SetBuffer(char *buffer, size_t size) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void IWrite(const char *buffer, size_t size, Status &status,
                size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    void IRead(char *buffer, size_t size, Status &status,
               size_t start = MaxSizeT) final;

    void Flush() final;

    void Close() final;

    void SetAddress(const std::string address);

private:
    void *m_Context = nullptr;
    void *m_Socket = nullptr;

    const std::string m_IPAddress;
    const std::string m_Port;
    std::string m_OpenModeStr;
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_ */
