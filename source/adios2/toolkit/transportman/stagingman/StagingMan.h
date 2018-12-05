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
    StagingMan(MPI_Comm mpiComm, Mode openMode, const int timeout,
               const bool debugMode);

    ~StagingMan();

    void OpenWriteTransport(std::string fullAddress);
    void OpenReadTransport();

    std::shared_ptr<std::vector<char>> Request(const std::vector<char> &request,
                                               const std::string &address,
                                               const size_t maxReplySize = 0);

    void ReceiveRequest(std::vector<char> &request,
                        const size_t maxRequestSize = 0);

    void SendReply(std::shared_ptr<std::vector<char>> reply);

private:
    bool GetBoolParameter(const Params &params, const std::string key);
    bool GetStringParameter(const Params &params, const std::string key,
                            std::string &value);
    bool GetIntParameter(const Params &params, const std::string key,
                         int &value);

    MPI_Comm m_MpiComm;
    int m_Timeout;
    Mode m_OpenMode;
    bool m_DebugMode;

    size_t m_MaxRequestSize = 1000000;
    size_t m_MaxReplySize = 1000000000;

    std::shared_ptr<transport::SocketZmqReqRep> m_Transport;
};

} // end namespace transportman
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_TRANSPORTMAN_STAGINGMAN_H_ */
