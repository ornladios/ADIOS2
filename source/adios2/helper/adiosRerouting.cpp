/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosRerouting.cpp
 *
 *  Created on: Sept 26, 2025
 *      Author: Scott Wittenburg scott.wittenburg@kitware.com
 */

#include "adiosRerouting.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosMemory.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
/// \endcond


namespace adios2
{
namespace helper
{

void RerouteMessage::ToBuffer(std::vector<char> &buffer)
{
    size_t pos = 0;
    buffer.resize(REROUTE_MESSAGE_SIZE);
    helper::CopyToBuffer(buffer, pos, &this->m_MsgType);
    helper::CopyToBuffer(buffer, pos, &this->m_SrcRank);
    helper::CopyToBuffer(buffer, pos, &this->m_DestRank);
    helper::CopyToBuffer(buffer, pos, &this->m_SubStreamIdx);
    helper::CopyToBuffer(buffer, pos, &this->m_Offset);
    helper::CopyToBuffer(buffer, pos, &this->m_Size);
}


void RerouteMessage::FromBuffer(const std::vector<char> &buffer)
{
    size_t pos = 0;
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_MsgType);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_SrcRank);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_DestRank);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_SubStreamIdx);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_Offset);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_Size);
}

void RerouteMessage::SendTo(helper::Comm& comm, int destRank)
{
    std::cout << "Rank " << comm.Rank() << " sending "
              << this->GetTypeString(this->m_MsgType) << " to " << destRank << std::endl;

    std::vector<char> sendBuf;
    this->ToBuffer(sendBuf);

    // You'd think send and/or recv should be non-blocking for this to work,
    // especially when sending to ourselves, but somehow blocking send and recv
    // seems to work.

    // Non-blocking send
    // comm.Isend(sendBuf.data(), sendBuf.size(), desinationtRank, 0);

    // "Blocking" send
    comm.Send(sendBuf.data(), sendBuf.size(), destRank, 0);
}

void RerouteMessage::RecvFrom(helper::Comm& comm, int srcRank)
{
    std::vector<char> recvBuf;
    recvBuf.resize(REROUTE_MESSAGE_SIZE);

    // Non-blocking receive
    // helper::Comm::Req req = comm.Irecv(recvBuf.data(), msgSize, recvFlag, 0);
    // helper::Comm::Status status = req.Wait();

    // "Blocking" receive
    helper::Comm::Status status = comm.Recv(recvBuf.data(), REROUTE_MESSAGE_SIZE, srcRank, 0);

    this->FromBuffer(recvBuf);

    std::cout << "Rank " << comm.Rank() << " received "
              << this->GetTypeString(this->m_MsgType) << " from " << status.Source
              << std::endl;

    if (status.Source != this->m_SrcRank)
    {
        std::cout << "   GAHHHHHHH!" << std::endl;
    }
}

} // end namespace helper
} // end namespace adios2
