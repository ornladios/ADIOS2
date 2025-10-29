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
#include <sstream>
#include <stdexcept>
#include <utility>
/// \endcond

namespace adios2
{
namespace helper
{

void RerouteMessage::NonBlockingSendTo(helper::Comm &comm, int destRank, std::vector<char> &buffer)
{
    size_t pos = 0;
    buffer.resize(REROUTE_MESSAGE_SIZE);
    helper::CopyToBuffer(buffer, pos, &this->m_MsgType);
    helper::CopyToBuffer(buffer, pos, &this->m_SrcRank);
    helper::CopyToBuffer(buffer, pos, &this->m_DestRank);
    helper::CopyToBuffer(buffer, pos, &this->m_SubStreamIdx);
    helper::CopyToBuffer(buffer, pos, &this->m_Offset);
    helper::CopyToBuffer(buffer, pos, &this->m_Size);

    std::stringstream ss;

    ss << "Rank " << comm.Rank() << " SEND buffer of length " << buffer.size() << ": [";
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        ss << " " << static_cast<int>(buffer[i]);
    }
    ss << " ]\n";

    std::cout << ss.str();

    comm.Isend(buffer.data(), buffer.size(), destRank, 0);
}

void RerouteMessage::BlockingRecvFrom(helper::Comm &comm, int srcRank, std::vector<char> &buffer)
{
    buffer.resize(REROUTE_MESSAGE_SIZE);
    comm.Recv(buffer.data(), REROUTE_MESSAGE_SIZE, srcRank, 0);

    std::stringstream ss;

    ss << "Rank " << comm.Rank() << " RECV buffer of length " << buffer.size() << ": [";
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        ss << " " << static_cast<int>(buffer[i]);
    }
    ss << " ]\n";

    std::cout << ss.str();

    size_t pos = 0;
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_MsgType);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_SrcRank);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_DestRank);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_SubStreamIdx);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_Offset);
    helper::CopyFromBuffer(buffer.data(), pos, &this->m_Size);
}

} // end namespace helper
} // end namespace adios2
