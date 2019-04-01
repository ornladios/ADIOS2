/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingMan.h
 *
 *  Created on: Oct 1, 2018
 *      Author: Jason Wang wangr1@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_
#define ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_

#include "adios2/core/IO.h"
#include "adios2/core/Operator.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/TransportMan.h"

#ifdef ADIOS2_HAVE_ZEROMQ
#include "adios2/toolkit/transport/socket/SocketZmqReqRep.h"
#endif

namespace adios2
{
namespace transportman
{

class StagingMan
{

public:
    StagingMan(const MPI_Comm mpiComm, const Mode openMode, const int timeout,
               const size_t maxBufferSize);

    ~StagingMan();

    void OpenTransport(const std::string &fullAddress);

    void CloseTransport();

    std::shared_ptr<std::vector<char>> Request(const std::vector<char> &request,
                                               const std::string &address);

    std::shared_ptr<std::vector<char>> ReceiveRequest();

    void SendReply(std::shared_ptr<std::vector<char>> reply);

private:
    MPI_Comm m_MpiComm;
    int m_Timeout;
    Mode m_OpenMode;
    int m_Verbosity = 0;

    size_t m_MaxBufferSize;

    std::vector<char> m_Buffer;

    transport::SocketZmqReqRep m_Transport;
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_ */
