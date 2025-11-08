/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.cpp
 *
 */

#include "BP5Writer.h"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/helper/adiosRerouting.h"
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include <adios2-perfstubs-interface.h>

#include <ctime>
#include <iostream>
#include <thread>

namespace
{
class BufferPool
{
public:
    BufferPool(int size) { m_Pool.resize(size); }

    ~BufferPool() = default;

    std::vector<char> &GetNextBuffer()
    {
        size_t bufferIdx = m_CurrentBufferIdx;

        if (m_CurrentBufferIdx < m_Pool.size() - 1)
        {
            m_CurrentBufferIdx += 1;
        }
        else
        {
            m_CurrentBufferIdx = 0;
        }

        return m_Pool[bufferIdx];
    }

    size_t m_CurrentBufferIdx = 0;
    std::vector<std::vector<char>> m_Pool;
};

struct WriterGroupState
{
    enum class Status
    {
        UNKNOWN,
        WRITING,
        IDLE,
        PENDING,
        CAPACITY,
        CLOSED,
    };

    Status m_currentStatus;
    int m_queueSize;
    size_t m_subFileIndex;
    // We could also keep track of number of times rerouted to/from
};

bool IsFinished(const WriterGroupState &state)
{
    return state.m_currentStatus == WriterGroupState::Status::CAPACITY ||
           state.m_currentStatus == WriterGroupState::Status::IDLE;
}

/// Avoid visiting the same rerouting source or destination groups
/// repeatedly
class StateTraversal
{
public:
    StateTraversal()
    {
        m_idlerIndex = 0;
        m_writerIndex = 0;
    }

    enum class SearchResult
    {
        NOT_FOUND,
        FOUND,
        FINISHED,
    };

    SearchResult FindNextPair(const std::vector<WriterGroupState> &state,
                              std::pair<size_t, size_t> &nextPair)
    {
        auto isIdle = [](WriterGroupState s) {
            return s.m_currentStatus == WriterGroupState::Status::IDLE;
        };

        SearchResult idleResult = GetNext(isIdle, state, m_idlerIndex, nextPair.first);

        if (idleResult != SearchResult::FOUND)
        {
            return idleResult;
        }

        auto canOffload = [](WriterGroupState s) {
            return s.m_currentStatus == WriterGroupState::Status::WRITING && s.m_queueSize > 0;
        };

        return GetNext(canOffload, state, m_writerIndex, nextPair.second);
    }

private:
    SearchResult GetNext(bool (*checkFn)(WriterGroupState),
                         const std::vector<WriterGroupState> &state, size_t &searchIndex,
                         size_t &foundIndex)
    {
        if (state.size() == 0)
        {
            return SearchResult::NOT_FOUND;
        }

        size_t checkedCount = 0;
        size_t finishedCount = 0;

        while (checkedCount < state.size())
        {
            checkedCount++;
            searchIndex++;

            if (searchIndex >= state.size())
            {
                searchIndex = 0;
            }

            if (checkFn(state[searchIndex]))
            {
                foundIndex = searchIndex;
                return SearchResult::FOUND;
            }
            else if (IsFinished(state[searchIndex]))
            {
                finishedCount++;
            }
        }

        if (finishedCount == state.size())
        {
            return SearchResult::FINISHED;
        }

        return SearchResult::NOT_FOUND;
    }

    size_t m_idlerIndex;
    size_t m_writerIndex;
};
}

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

void BP5Writer::ReroutingCommunicationLoop()
{
    using RerouteMessage = adios2::helper::RerouteMessage;

    std::cout << "Rank " << m_RankMPI << " Enter ReroutingCommunicationLoop" << std::endl;

    int subCoord = m_Aggregator->m_AggregatorRank;
    bool iAmSubCoord = m_RankMPI == subCoord;

    // Arbitrarily decide that the sub coordinator of the first partition is also
    // the global coordinator
    int globalCoord = m_Partitioning.m_Partitions[0][0];
    bool iAmGlobalCoord = m_RankMPI == globalCoord;

    // Some containers only used by augmented roles:

    // sub coordinators maintain a queue of writers
    std::queue<int> writerQueue;

    // global coordinator keeps track of state of each subcoord
    std::vector<WriterGroupState> groupState;
    bool needStateCheck = false;
    StateTraversal pairFinder;
    std::vector<int> subCoordRanks;
    bool firstIdleMsg = true;
    std::set<int> closeAcksNeeded;
    std::set<int> groupIdlesNeeded;
    bool waitingForCloseAcks = false;

    // Sends are non-blocking. We use the pool to avoid the situation where the
    // buffer is destructed before the send is complete.  If we start seeing
    // many Sends pending for a long time, that could cause us to exhaust the
    // pool of buffers, at which point we would start overwriting buffers in the
    // pool, potentially creating errors which are difficult to debug/diagnose.
    int bufferSize = 1;
    if (iAmGlobalCoord)
    {
        bufferSize = subCoordRanks.size();
    }
    else if (iAmSubCoord)
    {
        bufferSize = 100;
    }
    BufferPool sendBuffers((iAmSubCoord ? 100 : 1));
    std::vector<char> recvBuffer;
    int writingRank = -1;
    uint64_t currentFilePos = 0;
    bool sentFinished = false;
    bool receivedGroupClose = false;
    bool expectingWriteCompletion = false;
    bool sentIdle = false;

    if (iAmGlobalCoord)
    {
        groupState.resize(m_CommAggregators.Size());
        subCoordRanks.resize(m_CommAggregators.Size());

        for (size_t i = 0; i < m_Partitioning.m_Partitions.size(); ++i)
        {
            // Status remains unknown until we hear something specific (i.e. idle
            // message or status inquiry response)
            groupState[i].m_currentStatus = WriterGroupState::Status::UNKNOWN;
            groupState[i].m_subFileIndex = i;
            subCoordRanks[i] = m_Partitioning.m_Partitions[i][0];
            closeAcksNeeded.insert(subCoordRanks[i]);
            groupIdlesNeeded.insert(subCoordRanks[i]);
        }
    }

    if (iAmSubCoord)
    {
        // Pre-populate my queue with the ranks in my group/partition
        const std::vector<size_t> &groupRanks =
            m_Partitioning.m_Partitions[m_Aggregator->m_SubStreamIndex];
        for (auto rank : groupRanks)
        {
            writerQueue.push(static_cast<int>(rank));
        }

        currentFilePos = m_DataPos;

        if (m_DataPosShared)
        {
            // We have shared data pos after a previous timestep, we should update our
            // notion of m_DataPos
            currentFilePos = m_SubstreamDataPos[m_Aggregator->m_SubStreamIndex];
            m_DataPosShared = false;
        }
    }

    auto keepGoing = [&]()
    {
        if (iAmSubCoord)
        {
            return !receivedGroupClose || expectingWriteCompletion || waitingForCloseAcks;
        }

        return !receivedGroupClose;
    };

    while (keepGoing())
    {
        int msgReady = 0;
        helper::Comm::Status status =
            m_Comm.Iprobe(static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &msgReady);

        // Many messages may not result in a state change for the global coordinator,
        // so set this flag if a check is needed
        needStateCheck = false;

        // If there is a message ready, receive and handle it
        if (msgReady)
        {
            RerouteMessage message;
            message.BlockingRecvFrom(m_Comm, status.Source, recvBuffer);

            switch ((RerouteMessage::MessageType)message.m_MsgType)
            {
            case RerouteMessage::MessageType::DO_WRITE:
                std::cout << "Rank " << m_RankMPI << " received DO_WRITE" << std::endl;
                // msg for all processes
                {
                    std::unique_lock<std::mutex> lck(m_WriteMutex);
                    m_TargetIndex = message.m_WildCard;
                    m_DataPos = message.m_Offset;
                    // m_TargetCoordinator = message.m_SrcRank;
                    m_TargetCoordinator = status.Source;
                    m_ReadyToWrite = true;
                    m_WriteCV.notify_one();
                }
                break;
            case RerouteMessage::MessageType::WRITE_COMPLETION:
                std::cout << "Rank " << m_RankMPI << " received WRITE_COMPLETION from rank "
                          << status.Source << std::endl;
                // msg for sub coordinator
                currentFilePos = message.m_Offset;
                writingRank = -1;
                expectingWriteCompletion = false;
                break;
            case RerouteMessage::MessageType::GROUP_CLOSE:
                std::cout << "Rank " << m_RankMPI << " received GROUP_CLOSE from rank "
                          << status.Source << std::endl;
                // msg for sub coordinator
                m_DataPos = currentFilePos;
                receivedGroupClose = true;

                std::cout << "Rank " << m_RankMPI << " sending GROUP_CLOSE_ACK to rank "
                    << globalCoord << std::endl;
                adios2::helper::RerouteMessage closeAckMsg;
                closeAckMsg.m_MsgType = RerouteMessage::MessageType::GROUP_CLOSE_ACK;
                closeAckMsg.m_SrcRank = m_RankMPI;
                closeAckMsg.m_DestRank = globalCoord;
                closeAckMsg.m_WildCard = m_Aggregator->m_SubStreamIndex;
                closeAckMsg.BlockingSendTo(m_Comm, globalCoord,
                                            sendBuffers.GetNextBuffer());
                break;
            case RerouteMessage::MessageType::GROUP_CLOSE_ACK:
                // msg for global coordinator
                {
                    std::cout << "Rank " << m_RankMPI << " received GROUP_CLOSE_ACK from rank "
                              << status.Source << std::endl;

                    size_t ackGroupIdx = static_cast<size_t>(message.m_WildCard);
                    groupState[ackGroupIdx].m_currentStatus = WriterGroupState::Status::CLOSED;
                    closeAcksNeeded.erase(status.Source);

                    if (!closeAcksNeeded.empty())
                    {
                        std::stringstream ss;

                        ss << "Rank " << m_RankMPI << " still awaiting close acks from: [ ";
                        for (int n : closeAcksNeeded)
                        {
                            ss << n << " ";
                        }
                        ss << " ]\n";

                        std::cout << ss.str();
                    }
                }
                break;
            case RerouteMessage::MessageType::WRITER_IDLE:
                std::cout << "Rank " << m_RankMPI << " received WRITER_IDLE from rank "
                          << status.Source << std::endl;
                // msg for global coordinator
                {
                    // int idleWriter = message.m_SrcRank;
                    int idleWriter = status.Source;
                    size_t idleGroup = static_cast<size_t>(message.m_WildCard);
                    groupState[idleGroup].m_currentStatus = WriterGroupState::Status::IDLE;
                    groupState[idleGroup].m_queueSize = 0;
                    groupIdlesNeeded.erase(idleWriter);

                    if (firstIdleMsg)
                    {
                        for (size_t i = 0; i < subCoordRanks.size(); ++i)
                        {
                            int scRank = subCoordRanks[i];
                            // No need to ask the sender of the WRITER_IDLE msg how big their queue
                            // is
                            if (scRank != idleWriter)
                            {
                                std::cout << "GC (" << m_RankMPI << ") sending STATUS_INQUIRY to rank "
                                          << scRank << std::endl;
                                adios2::helper::RerouteMessage inquiryMsg;
                                inquiryMsg.m_MsgType = RerouteMessage::MessageType::STATUS_INQUIRY;
                                inquiryMsg.m_SrcRank = m_RankMPI;
                                inquiryMsg.m_DestRank = scRank;
                                inquiryMsg.NonBlockingSendTo(m_Comm, scRank,
                                                             sendBuffers.GetNextBuffer());
                            }
                        }

                        firstIdleMsg = false;
                    }

                    needStateCheck = true;
                }
                break;
            case RerouteMessage::MessageType::WRITER_CAPACITY:
                std::cout << "Rank " << m_RankMPI << " received WRITER_CAPACITY from rank "
                          << status.Source << std::endl;
                // msg for global coordinator
                {
                    int capacityGroup = message.m_WildCard;
                    groupState[capacityGroup].m_currentStatus = WriterGroupState::Status::CAPACITY;
                    needStateCheck = true;
                }
                break;
            case RerouteMessage::MessageType::STATUS_INQUIRY:
                std::cout << "Rank " << m_RankMPI << " received STATUS_INQUIRY from rank "
                          << status.Source << std::endl;
                // msg for sub coordinator
                adios2::helper::RerouteMessage replyMsg;
                replyMsg.m_MsgType = RerouteMessage::MessageType::STATUS_REPLY;
                replyMsg.m_SrcRank = m_RankMPI;
                replyMsg.m_DestRank = globalCoord;
                replyMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                replyMsg.m_Size = static_cast<uint64_t>(writerQueue.size());
                replyMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
                break;
            case RerouteMessage::MessageType::STATUS_REPLY:
                std::cout << "Rank " << m_RankMPI << " received STATUS_REPLY from rank "
                          << status.Source << std::endl;
                // msg for global coordinator
                {
                    size_t subStreamIdx = static_cast<size_t>(message.m_WildCard);
                    int qSize = static_cast<int>(message.m_Size);
                    groupState[subStreamIdx].m_queueSize = qSize;
                    groupState[subStreamIdx].m_currentStatus =
                        qSize > 0 ? WriterGroupState::Status::WRITING
                                  : WriterGroupState::Status::IDLE;
                    needStateCheck = true;
                }
                break;
            case RerouteMessage::MessageType::REROUTE_REQUEST:
                std::cout << "Rank " << m_RankMPI << " received REROUTE_REQUEST from rank "
                          << status.Source << std::endl;
                // msg for sub coordinator
                if (writerQueue.empty())
                {
                    std::cout << "Rank " << m_RankMPI << " sending REROUTE_REJECT to rank "
                              << globalCoord << std::endl;
                    adios2::helper::RerouteMessage rejectMsg;
                    rejectMsg.m_MsgType = RerouteMessage::MessageType::REROUTE_REJECT;
                    rejectMsg.m_SrcRank = message.m_SrcRank;
                    rejectMsg.m_DestRank = message.m_DestRank;
                    rejectMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
                }
                else
                {
                    std::cout << "Rank " << m_RankMPI << " sending REROUTE_ACK to rank "
                              << globalCoord << std::endl;
                    int reroutedRank = writerQueue.front();
                    writerQueue.pop();

                    adios2::helper::RerouteMessage ackMsg;
                    ackMsg.m_MsgType = RerouteMessage::MessageType::REROUTE_ACK;
                    ackMsg.m_SrcRank = message.m_SrcRank;
                    ackMsg.m_DestRank = message.m_DestRank;
                    ackMsg.m_WildCard = reroutedRank;
                    ackMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
                }
                break;
            case RerouteMessage::MessageType::REROUTE_REJECT:
                std::cout << "Rank " << m_RankMPI << " received REROUTE_REJECT from rank "
                          << status.Source << std::endl;
                // msg for global coordinator

                // Both the src and target subcoord states return from PENDING to their prior state
                if (groupState[message.m_SrcRank].m_currentStatus == WriterGroupState::Status::PENDING)
                {
                    groupState[message.m_SrcRank].m_currentStatus = WriterGroupState::Status::WRITING;
                    groupState[message.m_SrcRank].m_queueSize = 0;
                }

                if (groupState[message.m_DestRank].m_currentStatus == WriterGroupState::Status::PENDING)
                {
                    groupState[message.m_DestRank].m_currentStatus = WriterGroupState::Status::IDLE;
                }

                // The reason to check here is that global coord triggers at most
                // one reroute sequence per iteration through the loop. Otherwise
                // we probably don't need a state check upon receipt of rejection.
                needStateCheck = true;
                break;
            case RerouteMessage::MessageType::REROUTE_ACK:
                std::cout << "Rank " << m_RankMPI << " received REROUTE_ACK from rank "
                          << status.Source << std::endl;
                // msg for global coordinator

                std::cout << "Rank " << m_RankMPI << " sending WRITE_MORE to rank "
                          << message.m_DestRank << std::endl;

                // Send the lucky volunteer another writer
                adios2::helper::RerouteMessage writeMoreMsg;
                writeMoreMsg.m_MsgType = RerouteMessage::MessageType::WRITE_MORE;
                writeMoreMsg.m_WildCard = message.m_WildCard; // i.e. the rerouted writer rank
                writeMoreMsg.NonBlockingSendTo(m_Comm, message.m_DestRank,
                                               sendBuffers.GetNextBuffer());

                groupIdlesNeeded.insert(message.m_DestRank);

                // Src subcoord state is returned to writing, dest subcoord state is now writing as
                // well
                groupState[message.m_SrcRank].m_currentStatus = WriterGroupState::Status::WRITING;
                groupState[message.m_SrcRank].m_queueSize -= 1;
                groupState[message.m_DestRank].m_currentStatus = WriterGroupState::Status::WRITING;
                // groupState[message.m_DestRank].m_queueSize += 1;

                // The reason to check here is that global coord triggers at most
                // one reroute sequence per iteration through the loop. Otherwise
                // we probably don't need a state check upon receiving reroute ack.
                needStateCheck = true;
                break;
            case RerouteMessage::MessageType::WRITE_MORE:
                std::cout << "Rank " << m_RankMPI << " received WRITE_MORE from rank "
                          << status.Source << std::endl;
                // msg for sub coordinator
                writerQueue.push(message.m_WildCard);
                sentIdle = false;
                break;
            default:
                break;
            }
        }

        // All processes
        // Check if writing has finished, and alert the target SC
        if (!sentFinished)
        {
            // std::cout << "Rank " << m_RankMPI << " attempting to get lock on m_NotifMutex" << std::endl;
            std::lock_guard<std::mutex> lck(m_NotifMutex);
            if (m_FinishedWriting)
            {
                adios2::helper::RerouteMessage writeCompleteMsg;
                writeCompleteMsg.m_MsgType = RerouteMessage::MessageType::WRITE_COMPLETION;
                writeCompleteMsg.m_SrcRank = m_RankMPI;
                writeCompleteMsg.m_DestRank = m_TargetCoordinator;
                writeCompleteMsg.m_WildCard = m_TargetIndex;
                writeCompleteMsg.m_Offset = m_DataPos;

                if (!iAmSubCoord && !iAmGlobalCoord)
                {
                    std::cout << "Rank " << m_RankMPI << " notifying SC (" << m_TargetCoordinator
                            << ") of write completion -- BLOCKING" << std::endl;
                    // My only role was to write (no communication responsibility) so I am
                    // done at this point. However, I need to do a blocking send because I
                    // am about to return from this function, at which point my buffer pool
                    // goes away.
                    writeCompleteMsg.BlockingSendTo(m_Comm, m_TargetCoordinator,
                                                    sendBuffers.GetNextBuffer());

                    receivedGroupClose = true;
                    continue;
                }
                else
                {
                    std::cout << "Rank " << m_RankMPI << " notifying SC (" << m_TargetCoordinator
                            << ") of write completion -- NONBLOCKING" << std::endl;
                    writeCompleteMsg.NonBlockingSendTo(m_Comm, m_TargetCoordinator,
                                                    sendBuffers.GetNextBuffer());
                }

                sentFinished = true;
            }
        }

        // Subcoordinator processes
        // Check if anyone is writing right now, and if not, ask the next writer to start
        if (iAmSubCoord && writingRank == -1)
        {
            if (!writerQueue.empty())
            {
                // Pop the queue and send DO_WRITE
                int nextWriter = writerQueue.front();
                std::cout << "Rank " << m_RankMPI << " sending DO_WRITE to " << nextWriter << std::endl;
                writerQueue.pop();
                adios2::helper::RerouteMessage writeMsg;
                writeMsg.m_MsgType = RerouteMessage::MessageType::DO_WRITE;
                writeMsg.m_SrcRank = m_RankMPI;
                writeMsg.m_DestRank = nextWriter;
                writeMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                writeMsg.m_Offset = currentFilePos;
                writingRank = nextWriter;
                expectingWriteCompletion = true;
                writeMsg.NonBlockingSendTo(m_Comm, nextWriter, sendBuffers.GetNextBuffer());
            }
            else if (!sentIdle)
            {
                std::cout << "Rank " << m_RankMPI << " sending WRITER_IDLE to gc (" << globalCoord << ")" << std::endl;
                // Writer queue now empty, send WRITE_IDLE to the GC
                adios2::helper::RerouteMessage idleMsg;
                idleMsg.m_MsgType = RerouteMessage::MessageType::WRITER_IDLE;
                idleMsg.m_SrcRank = m_RankMPI;
                idleMsg.m_DestRank = globalCoord;
                idleMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                idleMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
                sentIdle = true;

                // TODO: If this group is already over the threshold ratio, send
                // TODO: WRITER_CAPACITY instead of WRITER_IDLE

                // adios2::helper::RerouteMessage capacityMsg;
                // capacityMsg.m_MsgType = RerouteMessage::MessageType::WRITER_CAPACITY;
                // // capacityMsg.m_SrcRank = m_RankMPI;
                // // capacityMsg.m_DestRank = globalCoord;
                // capacityMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                // capacityMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
            }
        }

        // Global coordinator process
        // Look for possible reroute-to / reroute-from pairs
        if (iAmGlobalCoord && needStateCheck && !waitingForCloseAcks)
        {
            std:: cout << "GC (" << m_RankMPI << ") looking for reroute candidate pair" << std::endl;
            std::pair<size_t, size_t> nextPair;
            StateTraversal::SearchResult result = pairFinder.FindNextPair(groupState, nextPair);

            if (result == StateTraversal::SearchResult::FOUND)
            {
                std:: cout << "GC (" << m_RankMPI << ") found reroute candidate pair (" << nextPair.first
                           << ", " << nextPair.second << "), sending REROUTE_REQUEST" << std::endl;
                // Finding a pair means there was both an idle group and a writing
                // group with at least one writer in its queue. With these, we will
                // initiate a reroute sequence.
                size_t idleIdx = nextPair.first;
                int idleSubcoordRank = subCoordRanks[idleIdx];
                size_t writerIdx = nextPair.second;
                int writerSubcoordRank = subCoordRanks[writerIdx];

                adios2::helper::RerouteMessage rerouteReqMsg;
                rerouteReqMsg.m_MsgType = RerouteMessage::MessageType::REROUTE_REQUEST;
                rerouteReqMsg.m_SrcRank = writerSubcoordRank;
                rerouteReqMsg.m_DestRank = idleSubcoordRank;
                rerouteReqMsg.NonBlockingSendTo(m_Comm, writerSubcoordRank,
                                                sendBuffers.GetNextBuffer());

                groupState[idleIdx].m_currentStatus = WriterGroupState::Status::PENDING;
                groupState[writerIdx].m_currentStatus = WriterGroupState::Status::PENDING;
            }
            else if (result == StateTraversal::SearchResult::FINISHED && groupIdlesNeeded.empty())
            {

                for (size_t i = 0; i < groupState.size(); ++i)
                {
                    std::cout << "  group " << i << " status: " << static_cast<int>(groupState[i].m_currentStatus)
                              << ", queue size: " << groupState[i].m_queueSize << std::endl;
                }
                // If we didn't find a pair, it could be because all the groups are
                // done writing (either idle or possibly at capacity). In that case,
                // we need to release the subcoordinators (and ourself) from their
                // comm loop.
                for (size_t scIdx = 0; scIdx < subCoordRanks.size(); ++scIdx)
                {
                    if (subCoordRanks[scIdx] != globalCoord)
                    {
                        std:: cout << "Rank " << m_RankMPI << " sending GROUP_CLOSE to rank " << subCoordRanks[scIdx] << std::endl;
                        adios2::helper::RerouteMessage closeMsg;
                        closeMsg.m_MsgType = RerouteMessage::MessageType::GROUP_CLOSE;
                        closeMsg.NonBlockingSendTo(m_Comm, subCoordRanks[scIdx], sendBuffers.GetNextBuffer());
                    }
                }

                std::cout << "Rank " << m_RankMPI << " marking my own close ack as received" << std::endl;
                receivedGroupClose = true;
                closeAcksNeeded.erase(globalCoord);

                waitingForCloseAcks = true;
            }
        }

        if (iAmGlobalCoord)
        {
            if (waitingForCloseAcks && closeAcksNeeded.empty())
            {
                std::cout << "Rank " << m_RankMPI << " got all my close acks" << std::endl;
                // global coordinator received the final close ack, now it can leave
                waitingForCloseAcks = false;
            }
            else
            {
                std::cout << "Rank " << m_RankMPI << " still need " << closeAcksNeeded.size() << " close acks [ ";
                for (int n : closeAcksNeeded)
                {
                    std::cout << " " << n;
                }
                std::cout << " ]" << std::endl;
            }
        }
    }

    std::cout << "Rank " << m_RankMPI << " Exit ReroutingCommunicationLoop" << std::endl;
}

void BP5Writer::WriteData_WithRerouting(format::BufferV *Data)
{
    // - start the communcation loop running in a thread
    // - wait to be signalled by the communication thread, then write:
    //       - variables set by comm thread indicate subfile and offset
    // - attempt to join the comm thread
    // - return

    // The communcation function:
    //
    // - Send a message to your SC, requesting to write
    // - enter the message loop where you:
    //       - check if a message is ready, and if so:
    //          - receive the message
    //          - handle the message, based on your role and the sender
    //       - if no message is ready, check if the writing is finished, and if so:
    //          - send a message to SC telling them:
    //              - you're done
    //              - the new offset (or the amount you wrote)

    m_ReadyToWrite = false;
    m_FinishedWriting = false;

    std::thread commThread(&BP5Writer::ReroutingCommunicationLoop, this);

    std::cout << "Background thread for rank " << m_RankMPI << " is now running" << std::endl;

    // wait until communication thread indicates it's our turn to write
    {
        std::unique_lock<std::mutex> lck(m_WriteMutex);
        m_WriteCV.wait(lck, [this] { return m_ReadyToWrite; });
    }

    // Do the writing

    std::cout << "Rank " << m_RankMPI << " signaled to write" << std::endl;

    // Check if we need to update which file we are writing to
    if (m_TargetIndex != m_Aggregator->m_SubStreamIndex)
    {
        // We were rerouted! Our aggregator subfile index is later exchanged with other
        // ranks to be written to metadata, so update it here or the metadata will be
        // wrong.
        m_Aggregator->m_SubStreamIndex = m_TargetIndex;

        // Open the subfile without doing any collective communications, since the global
        // coordinator ensures only one rerouted rank opens this file at a time.
        OpenSubfile(false);
    }

    // align to PAGE_SIZE
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos, m_Parameters.StripeSize);
    m_StartDataPos = m_DataPos;

    std::vector<core::iovec> DataVec = Data->DataVec();

    AggTransportData &aggData = m_AggregatorSpecifics.at(GetCacheKey(m_Aggregator));
    aggData.m_FileDataManager.WriteFileAt(DataVec.data(), DataVec.size(), m_StartDataPos);

    m_DataPos += Data->Size();

    // Now signal the communication thread that this rank has finished writing
    {
        std::lock_guard<std::mutex> lck(m_NotifMutex);
        m_FinishedWriting = true;
    }

    commThread.join();

    // std::cout << "Background thread for rank " << m_RankMPI << " is now finished" << std::endl;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
