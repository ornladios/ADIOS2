/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosRerouting.h helpers for BP5 rerouting aggregation
 *
 *  Created on: Sept 26, 2025
 *      Author: Scott Wittenburg scott.wittenburg@kitware.com
 */

#ifndef ADIOS2_HELPER_ADIOSREROUTING_H_
#define ADIOS2_HELPER_ADIOSREROUTING_H_

#include "adios2/helper/adiosComm.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstddef>
#include <cstdint>
#include <vector>
/// \endcond

namespace adios2
{

namespace helper
{

class RerouteMessage
{
public:
    enum class MessageType
    {
        DO_WRITE,
        WRITER_CAPACITY,
        WRITER_IDLE,
        WRITE_MORE,
        WRITE_COMPLETION,
        REROUTE_REQUEST,
        REROUTE_ACK,
        REROUTE_REJECT,
        GROUP_CLOSE,
        STATUS_INQUIRY,
        STATUS_REPLY,
    };

    std::string GetTypeString(MessageType mtype)
    {
        switch (mtype)
        {
        case MessageType::DO_WRITE:
            return std::string("DO_WRITE");
            break;
        case MessageType::WRITER_IDLE:
            return std::string("WRITER_IDLE");
            break;
        case MessageType::WRITE_MORE:
            return std::string("WRITE_MORE");
            break;
        case MessageType::WRITE_COMPLETION:
            return std::string("WRITE_COMPLETION");
            break;
        default:
            return std::string("UNKNOWN");
            break;
        }
    }

    // Send the contents of this message to another rank
    void NonBlockingSendTo(helper::Comm &comm, int destRank, std::vector<char> &buffer);

    // Receive a message from another rank to populate this message
    void BlockingRecvFrom(helper::Comm &comm, int srcRank, std::vector<char> &buffer);

    MessageType m_MsgType;
    int m_SrcRank;
    int m_DestRank;
    int m_WildCard;
    uint64_t m_Offset;
    uint64_t m_Size;

    static const size_t REROUTE_MESSAGE_SIZE = sizeof(MessageType) + sizeof(int) + sizeof(int) +
                                               sizeof(int) + sizeof(uint64_t) + sizeof(uint64_t);
};

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSREROUTING_H_ */
