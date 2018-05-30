/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * WANZmq.h
 *
 *  Created on: May 26, 2017
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_
#define ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_

#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

class WANZmq : public Transport
{

public:
    /**
     *
     * @param ipAddress
     * @param port
     * @param mpiComm
     * @param debugMode
     */
    WANZmq(const std::string ipAddress, const std::string port,
           MPI_Comm mpiComm, const std::string transportMode,
           const bool debugMode);

    ~WANZmq();

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
    const std::string m_IPAddress;
    const std::string m_Port;
    const std::string m_TransportMode;

    /** context handler created by zmq, thread safe */
    void *m_Context = nullptr;

    /** socket handler created by zmq */
    void *m_Socket = nullptr;

    int OpenSubscribe(const std::string &name, const Mode openMode,
                      const std::string ip);
    int OpenPush(const std::string &name, const Mode openMode,
                 const std::string ip);
    int OpenQuery(const std::string &name, const Mode openMode,
                  const std::string ip);

    void WritePubSub(const char *buffer, size_t size, const bool blocking);
    void WriteSenderDriven(const char *buffer, size_t size,
                           const bool blocking);
    void WriteReceiverDriven(const char *buffer, size_t size,
                             const bool blocking);
};

} // end namespace transport
} // end namespace adios

#endif /* ADIOS2_TOOLKIT_TRANSPORT_WAN_WANZMQ_H_ */
