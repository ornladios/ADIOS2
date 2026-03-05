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

#include <chrono>
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
        WRITING_MORE,
        REROUTE_REJECTED,
        IDLE,
        PENDING,
        CAPACITY,
        CLOSED,
    };

    Status m_currentStatus;
    size_t m_subFileIndex;
};

bool IsFinished(const WriterGroupState &state)
{
    return state.m_currentStatus == WriterGroupState::Status::CAPACITY ||
           state.m_currentStatus == WriterGroupState::Status::IDLE;
}

// The purpose of this class is to try to find an idle group and a busy
// writing group, in such a way that we distribute the rerouting work
// more or less fairly to the faster writers.
class StateTraversal
{
public:
    StateTraversal()
    {
        // These track where we last left off looking for idle and busy
        // groups, so that next time we seek a reroute candidate pair, we
        // resume looking from those points, independently.
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
            return s.m_currentStatus == WriterGroupState::Status::WRITING;
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

        // Make sure we don't just go around and around the state repeatedly
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

    if (m_Parameters.verbose > 3)
    {
        std::cout << "Rank " << m_RankMPI << " Enter ReroutingCommunicationLoop" << std::endl;
    }

    m_Profiler.AddTimerWatch("Rerouting_Send_NB");
    m_Profiler.AddTimerWatch("Rerouting_Send_B");
    m_Profiler.AddTimerWatch("Rerouting_Recv_NB");

    auto lf_SendNonBlocking = [](RerouteMessage &msg, adios2::helper::Comm &comm, int toRank,
                                 std::vector<char> &buffer,
                                 adios2::profiling::JSONProfiler &profiler) {
        profiler.Start("Rerouting_Send_NB");
        msg.NonBlockingSendTo(comm, toRank, buffer);
        profiler.Stop("Rerouting_Send_NB");
    };

    auto lf_SendBlocking = [](RerouteMessage &msg, adios2::helper::Comm &comm, int toRank,
                              std::vector<char> &buffer,
                              adios2::profiling::JSONProfiler &profiler) {
        profiler.Start("Rerouting_Send_B");
        msg.BlockingSendTo(comm, toRank, buffer);
        profiler.Stop("Rerouting_Send_B");
    };

    auto lf_RecvBlocking = [](RerouteMessage &msg, adios2::helper::Comm &comm, int fromRank,
                              std::vector<char> &buffer,
                              adios2::profiling::JSONProfiler &profiler) {
        profiler.Start("Rerouting_Recv_NB");
        msg.BlockingRecvFrom(comm, fromRank, buffer);
        profiler.Stop("Rerouting_Recv_NB");
    };

    int subCoord = m_Aggregator->m_AggregatorRank;
    bool iAmSubCoord = m_RankMPI == subCoord;

    // Arbitrarily decide that the sub coordinator of the first partition is also
    // the global coordinator
    int globalCoord = static_cast<int>(m_Partitioning.m_Partitions[0][0]);
    bool iAmGlobalCoord = m_RankMPI == globalCoord;

    // sub coordinators maintain a queue of writers
    std::queue<int> writerQueue;

    // global coordinator keeps track of state of each subcoord
    std::vector<WriterGroupState> groupState;
    StateTraversal pairFinder;
    std::vector<int> subCoordRanks;
    std::map<int, size_t> scRankToIndex;
    bool firstIdleMsg = true;
    std::set<int> closeAcksNeeded;
    std::set<int> groupIdlesNeeded;
    bool waitingForCloseAcks = false;

    // Most sends are currently non-blocking. We use the pool to avoid a
    // situation where the buffer is destructed before the send is complete.
    BufferPool sendBuffers(100);
    std::vector<char> recvBuffer;
    int writingRank = -1;
    uint64_t currentFilePos = 0;
    uint64_t writeMoreCount = 0;
    uint64_t expectedOffset = 0;
    double thresholdFactor = static_cast<double>(m_Parameters.ReroutingThresholdFactor);
    bool atCapacity = false;
    bool sentFinished = false;
    bool receivedGroupClose = false;
    bool expectingWriteCompletion = false;
    bool sentIdle = false;

    if (iAmGlobalCoord)
    {
        // Global coordinator initializes the state it tracks for each subcoord
        groupState.resize(m_CommAggregators.Size());
        subCoordRanks.resize(m_CommAggregators.Size());

        for (size_t i = 0; i < m_Partitioning.m_Partitions.size(); ++i)
        {
            // Status remains unknown until we hear something specific (i.e. idle
            // message or status inquiry response)
            groupState[i].m_currentStatus = WriterGroupState::Status::UNKNOWN;
            groupState[i].m_subFileIndex = i;
            subCoordRanks[i] = static_cast<int>(m_Partitioning.m_Partitions[i][0]);
            scRankToIndex[subCoordRanks[i]] = i;
            closeAcksNeeded.insert(subCoordRanks[i]);
            groupIdlesNeeded.insert(subCoordRanks[i]);
        }
    }

    if (iAmSubCoord)
    {
        // Pre-populate my queue with the ranks in my group/partition, this avoids
        // the need for each writer to send a write request to the subcoord
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

    auto lf_keepGoing = [&]() {
        if (iAmSubCoord)
        {
            return !receivedGroupClose || expectingWriteCompletion || waitingForCloseAcks;
        }

        return !receivedGroupClose;
    };

    while (lf_keepGoing())
    {
        int msgReady = 0;
        helper::Comm::Status status =
            m_Comm.Iprobe(static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &msgReady);

        // If there is a message ready, receive and handle it
        if (msgReady)
        {
            RerouteMessage message;
            lf_RecvBlocking(message, m_Comm, status.Source, recvBuffer, m_Profiler);

            switch ((RerouteMessage::MessageType)message.m_MsgType)
            {
            case RerouteMessage::MessageType::DO_WRITE:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received DO_WRITE" << std::endl;
                }
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
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received WRITE_COMPLETION from rank "
                              << status.Source << std::endl;
                }
                // msg for sub coordinator
                currentFilePos = message.m_Offset;
                writingRank = -1;
                expectingWriteCompletion = false;

                if (writeMoreCount > 0)
                {
                    // I have written more than originally scheduled, check if I am "at capacity"
                    double ratio =
                        (currentFilePos - expectedOffset) / static_cast<double>(expectedOffset);

                    if (ratio >= thresholdFactor)
                    {
                        atCapacity = true;

                        if (m_Parameters.verbose > 2)
                        {
                            std::cout << "Subfile " << scRankToIndex[m_RankMPI] << " is now at "
                                      << "capacity (ratio = " << ratio
                                      << ", threshold factor = " << thresholdFactor << ")"
                                      << std::endl;
                        }
                    }
                }
                else
                {
                    // I am still working through my originally scheduled workload
                    expectedOffset = currentFilePos;
                }

                break;
            case RerouteMessage::MessageType::GROUP_CLOSE:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received GROUP_CLOSE from rank "
                              << status.Source << std::endl;
                }
                // msg for sub coordinator
                receivedGroupClose = true;

                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " sending GROUP_CLOSE_ACK to rank "
                              << globalCoord << std::endl;
                }
                adios2::helper::RerouteMessage closeAckMsg;
                closeAckMsg.m_MsgType = RerouteMessage::MessageType::GROUP_CLOSE_ACK;
                closeAckMsg.m_SrcRank = m_RankMPI;
                closeAckMsg.m_DestRank = globalCoord;
                closeAckMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                lf_SendBlocking(closeAckMsg, m_Comm, globalCoord, sendBuffers.GetNextBuffer(),
                                m_Profiler);
                break;
            case RerouteMessage::MessageType::GROUP_CLOSE_ACK:
                // msg for global coordinator
                {
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " received GROUP_CLOSE_ACK from rank "
                                  << status.Source << std::endl;
                    }

                    size_t ackGroupIdx = static_cast<size_t>(message.m_WildCard);
                    groupState[ackGroupIdx].m_currentStatus = WriterGroupState::Status::CLOSED;
                    closeAcksNeeded.erase(status.Source);
                }
                break;
            case RerouteMessage::MessageType::WRITER_IDLE:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received WRITER_IDLE from rank "
                              << status.Source << std::endl;
                }
                // msg for global coordinator
                {
                    int idleWriter = status.Source;
                    size_t idleGroup = static_cast<size_t>(message.m_WildCard);
                    groupState[idleGroup].m_currentStatus = WriterGroupState::Status::IDLE;
                    groupIdlesNeeded.erase(idleWriter);

                    if (firstIdleMsg)
                    {
                        // After receiving the first idle message from a subcoord, send a
                        // status inquiry to the other subcoords.
                        for (size_t i = 0; i < subCoordRanks.size(); ++i)
                        {
                            int scRank = subCoordRanks[i];
                            // No need to query the sender of the WRITER_IDLE msg for status
                            if (scRank != idleWriter)
                            {
                                if (m_Parameters.verbose > 3)
                                {
                                    std::cout << "GC (" << m_RankMPI
                                              << ") sending STATUS_INQUIRY to rank " << scRank
                                              << std::endl;
                                }
                                adios2::helper::RerouteMessage inquiryMsg;
                                inquiryMsg.m_MsgType = RerouteMessage::MessageType::STATUS_INQUIRY;
                                inquiryMsg.m_SrcRank = m_RankMPI;
                                inquiryMsg.m_DestRank = scRank;
                                lf_SendNonBlocking(inquiryMsg, m_Comm, scRank,
                                                   sendBuffers.GetNextBuffer(), m_Profiler);
                            }
                        }

                        firstIdleMsg = false;
                    }
                }
                break;
            case RerouteMessage::MessageType::WRITER_CAPACITY:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received WRITER_CAPACITY from rank "
                              << status.Source << std::endl;
                }
                // msg for global coordinator
                {
                    int capacityWriter = status.Source;
                    int capacityGroup = message.m_WildCard;
                    groupState[capacityGroup].m_currentStatus = WriterGroupState::Status::CAPACITY;
                    // The source SC is at capacity, we don't expect a WRITER_IDLE msg from it
                    groupIdlesNeeded.erase(capacityWriter);
                }
                break;
            case RerouteMessage::MessageType::STATUS_INQUIRY:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received STATUS_INQUIRY from rank "
                              << status.Source << std::endl;
                }
                // msg for sub coordinator
                adios2::helper::RerouteMessage replyMsg;
                replyMsg.m_MsgType = RerouteMessage::MessageType::STATUS_REPLY;
                replyMsg.m_SrcRank = m_RankMPI;
                replyMsg.m_DestRank = globalCoord;
                // The response to the status query is my subfile and queue size
                replyMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                replyMsg.m_Size = static_cast<uint64_t>(writerQueue.size());
                lf_SendNonBlocking(replyMsg, m_Comm, globalCoord, sendBuffers.GetNextBuffer(),
                                   m_Profiler);
                break;
            case RerouteMessage::MessageType::STATUS_REPLY:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received STATUS_REPLY from rank "
                              << status.Source << std::endl;
                }
                // msg for global coordinator
                {
                    size_t subStreamIdx = static_cast<size_t>(message.m_WildCard);
                    int qSize = static_cast<int>(message.m_Size);
                    groupState[subStreamIdx].m_currentStatus =
                        qSize > 0 ? WriterGroupState::Status::WRITING
                                  : WriterGroupState::Status::IDLE;
                }
                break;
            case RerouteMessage::MessageType::REROUTE_REQUEST:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received REROUTE_REQUEST from rank "
                              << status.Source << std::endl;
                }
                // msg for sub coordinator
                if (writerQueue.empty())
                {
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " sending REROUTE_REJECT to rank "
                                  << globalCoord << std::endl;
                    }
                    adios2::helper::RerouteMessage rejectMsg;
                    rejectMsg.m_MsgType = RerouteMessage::MessageType::REROUTE_REJECT;
                    rejectMsg.m_SrcRank = message.m_SrcRank;
                    rejectMsg.m_DestRank = message.m_DestRank;
                    lf_SendNonBlocking(rejectMsg, m_Comm, globalCoord, sendBuffers.GetNextBuffer(),
                                       m_Profiler);
                }
                else
                {
                    int reroutedRank = writerQueue.front();
                    writerQueue.pop();

                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " sending REROUTE_ACK to rank "
                                  << globalCoord << " (rerouted rank = " << reroutedRank << ")"
                                  << std::endl;
                    }

                    adios2::helper::RerouteMessage ackMsg;
                    ackMsg.m_MsgType = RerouteMessage::MessageType::REROUTE_ACK;
                    ackMsg.m_SrcRank = message.m_SrcRank;
                    ackMsg.m_DestRank = message.m_DestRank;
                    ackMsg.m_WildCard = reroutedRank;
                    lf_SendNonBlocking(ackMsg, m_Comm, globalCoord, sendBuffers.GetNextBuffer(),
                                       m_Profiler);
                }
                break;
            case RerouteMessage::MessageType::REROUTE_REJECT:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received REROUTE_REJECT from rank "
                              << status.Source << std::endl;
                }
                {
                    // msg for global coordinator
                    size_t srcIdx = scRankToIndex[message.m_SrcRank];
                    size_t destIdx = scRankToIndex[message.m_DestRank];

                    // Both the src and target subcoord states were put in a PENDING state when
                    // the reroute request was originally sent, now we have an answer (reject),
                    // so we need to update both states, assuming they are still PENDING.
                    if (groupState[srcIdx].m_currentStatus == WriterGroupState::Status::PENDING)
                    {
                        // A subcoord sends the rejection if it's queue is empty. After that
                        // point, it will never have work to donate again. So don't set the
                        // state to WRITING, because there's no need to both this group with
                        // any more reroute requests.
                        groupState[srcIdx].m_currentStatus =
                            WriterGroupState::Status::REROUTE_REJECTED;
                    }

                    if (groupState[destIdx].m_currentStatus == WriterGroupState::Status::PENDING)
                    {
                        groupState[destIdx].m_currentStatus = WriterGroupState::Status::IDLE;
                    }
                }
                break;
            case RerouteMessage::MessageType::REROUTE_ACK:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received REROUTE_ACK from rank "
                              << status.Source << std::endl;
                }
                // msg for global coordinator
                {
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " sending WRITE_MORE to rank "
                                  << message.m_DestRank << std::endl;
                    }

                    // Send the lucky volunteer another writer
                    adios2::helper::RerouteMessage writeMoreMsg;
                    writeMoreMsg.m_MsgType = RerouteMessage::MessageType::WRITE_MORE;
                    writeMoreMsg.m_WildCard = message.m_WildCard; // i.e. the rerouted writer rank
                    lf_SendNonBlocking(writeMoreMsg, m_Comm, message.m_DestRank,
                                       sendBuffers.GetNextBuffer(), m_Profiler);

                    groupIdlesNeeded.insert(message.m_DestRank);

                    // Update source and destination subcoord states
                    size_t srcIdx = scRankToIndex[message.m_SrcRank];
                    size_t destIdx = scRankToIndex[message.m_DestRank];
                    if (groupState[srcIdx].m_currentStatus == WriterGroupState::Status::PENDING)
                    {
                        // If I didn't receive a msg causing me to update this groups status
                        // in the interim (since I sent it REROUTE_REQUEST)), update it to
                        // WRITING now.
                        groupState[srcIdx].m_currentStatus = WriterGroupState::Status::WRITING;
                    }

                    // Don't set the dest group state to WRITING, because there's no need to
                    // bother it with reroute requests once it has started taking WRITE_MORE
                    // requests.
                    groupState[destIdx].m_currentStatus = WriterGroupState::Status::WRITING_MORE;
                }
                break;
            case RerouteMessage::MessageType::WRITE_MORE:
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " received WRITE_MORE from rank "
                              << status.Source << std::endl;
                }
                // msg for sub coordinator
                writerQueue.push(message.m_WildCard);
                sentIdle = false;
                writeMoreCount += 1;
                break;
            default:
                break;
            }
        } // end handling of incoming messages

        // All processes
        // Check if writing has finished, and alert the target SC
        if (!sentFinished)
        {
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
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " notifying SC ("
                                  << m_TargetCoordinator << ") of write completion -- BLOCKING"
                                  << std::endl;
                    }
                    // My only role was to write (no communication responsibility) so I am
                    // done at this point. However, I need to do a blocking send because I
                    // am about to return from this function, at which point my buffer pool
                    // goes away.
                    lf_SendBlocking(writeCompleteMsg, m_Comm, m_TargetCoordinator,
                                    sendBuffers.GetNextBuffer(), m_Profiler);

                    receivedGroupClose = true;
                    continue;
                }
                else
                {
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " notifying SC ("
                                  << m_TargetCoordinator << ") of write completion -- NONBLOCKING"
                                  << std::endl;
                    }
                    lf_SendNonBlocking(writeCompleteMsg, m_Comm, m_TargetCoordinator,
                                       sendBuffers.GetNextBuffer(), m_Profiler);
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
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " sending DO_WRITE to " << nextWriter
                              << std::endl;
                }
                writerQueue.pop();
                adios2::helper::RerouteMessage writeMsg;
                writeMsg.m_MsgType = RerouteMessage::MessageType::DO_WRITE;
                writeMsg.m_SrcRank = m_RankMPI;
                writeMsg.m_DestRank = nextWriter;
                writeMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                writeMsg.m_Offset = currentFilePos;
                writingRank = nextWriter;
                expectingWriteCompletion = true;
                lf_SendNonBlocking(writeMsg, m_Comm, nextWriter, sendBuffers.GetNextBuffer(),
                                   m_Profiler);
            }
            else if (!sentIdle)
            {
                // My writer queue is empty and I haven't yet communicated that to the GC
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " sending WRITER_IDLE to gc ("
                              << globalCoord << ")" << std::endl;
                }
                // Let the GC know I'm either able to take more work (by sending WRITER_IDLE)
                // or I've crossed the threshold factor and can no longer take more work (by
                // sending WRITER_CAPACITY).
                adios2::helper::RerouteMessage idleMsg;
                if (atCapacity)
                {
                    idleMsg.m_MsgType = RerouteMessage::MessageType::WRITER_CAPACITY;
                }
                else
                {
                    idleMsg.m_MsgType = RerouteMessage::MessageType::WRITER_IDLE;
                }
                idleMsg.m_SrcRank = m_RankMPI;
                idleMsg.m_DestRank = globalCoord;
                idleMsg.m_WildCard = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                lf_SendNonBlocking(idleMsg, m_Comm, globalCoord, sendBuffers.GetNextBuffer(),
                                   m_Profiler);
                sentIdle = true;
            }
        }

        // Global coordinator process
        // Look for possible reroute-to / reroute-from pairs
        if (iAmGlobalCoord && !waitingForCloseAcks)
        {
            if (m_Parameters.verbose > 4)
            {
                std::cout << "GC (" << m_RankMPI << ") looking for reroute candidate pair"
                          << std::endl;
            }
            std::pair<size_t, size_t> nextPair;
            StateTraversal::SearchResult result = pairFinder.FindNextPair(groupState, nextPair);

            if (result == StateTraversal::SearchResult::FOUND)
            {
                // Finding a pair means there was both an idle group and a writing
                // group. With these, we will initiate a reroute sequence.
                if (m_Parameters.verbose > 3)
                {
                    std::cout << "GC (" << m_RankMPI << ") found reroute candidate pair ("
                              << nextPair.first << ", " << nextPair.second
                              << "), sending REROUTE_REQUEST" << std::endl;
                }

                size_t idleIdx = nextPair.first;
                int idleSubcoordRank = subCoordRanks[idleIdx];
                size_t writerIdx = nextPair.second;
                int writerSubcoordRank = subCoordRanks[writerIdx];

                adios2::helper::RerouteMessage rerouteReqMsg;
                rerouteReqMsg.m_MsgType = RerouteMessage::MessageType::REROUTE_REQUEST;
                rerouteReqMsg.m_SrcRank = writerSubcoordRank;
                rerouteReqMsg.m_DestRank = idleSubcoordRank;
                lf_SendNonBlocking(rerouteReqMsg, m_Comm, writerSubcoordRank,
                                   sendBuffers.GetNextBuffer(), m_Profiler);

                groupState[idleIdx].m_currentStatus = WriterGroupState::Status::PENDING;
                groupState[writerIdx].m_currentStatus = WriterGroupState::Status::PENDING;
            }
            else if (result == StateTraversal::SearchResult::FINISHED && groupIdlesNeeded.empty())
            {
                // We didn't find a reroute candidate pair because all the groups are
                // done writing, so we can release the subcoords (and ourself) from the
                // communication thread.
                if (m_Parameters.verbose > 3)
                {
                    for (size_t i = 0; i < groupState.size(); ++i)
                    {
                        std::cout << "  group " << i
                                  << " status: " << static_cast<int>(groupState[i].m_currentStatus)
                                  << std::endl;
                    }
                }

                for (size_t scIdx = 0; scIdx < subCoordRanks.size(); ++scIdx)
                {
                    // Skip sending the group close message to ourself, we'll just ack that
                    // directly, below.
                    if (subCoordRanks[scIdx] != globalCoord)
                    {
                        if (m_Parameters.verbose > 3)
                        {
                            std::cout << "Rank " << m_RankMPI << " sending GROUP_CLOSE to rank "
                                      << subCoordRanks[scIdx] << std::endl;
                        }
                        adios2::helper::RerouteMessage closeMsg;
                        closeMsg.m_MsgType = RerouteMessage::MessageType::GROUP_CLOSE;
                        lf_SendNonBlocking(closeMsg, m_Comm, subCoordRanks[scIdx],
                                           sendBuffers.GetNextBuffer(), m_Profiler);
                    }
                }

                if (m_Parameters.verbose > 3)
                {
                    std::cout << "Rank " << m_RankMPI << " marking my own close ack as received"
                              << std::endl;
                }
                receivedGroupClose = true;
                closeAcksNeeded.erase(globalCoord);

                waitingForCloseAcks = true;
            }
            else
            {
                // Debugging block only. We didn't find a reroute candidate pair for some
                // other reason, so dump some info about what we did find.
                if (m_Parameters.verbose > 4)
                {
                    std::cout << "candidate pair search returned ";

                    switch (result)
                    {
                    case StateTraversal::SearchResult::NOT_FOUND:
                        std::cout << "NOT FOUND" << std::endl;
                        std::cout << "  states: [ ";
                        for (size_t i = 0; i < groupState.size(); ++i)
                        {
                            std::cout << static_cast<int>(groupState[i].m_currentStatus) << " ";
                        }
                        std::cout << "]" << std::endl;
                        break;
                    case StateTraversal::SearchResult::FOUND:
                        std::cout << "FOUND" << std::endl;
                        break;
                    case StateTraversal::SearchResult::FINISHED:
                        std::cout << "FINISHED" << std::endl;
                        break;
                    }
                }
            }
        }

        if (iAmGlobalCoord)
        {
            if (waitingForCloseAcks)
            {
                if (closeAcksNeeded.empty())
                {
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " got all my close acks" << std::endl;
                    }
                    // global coordinator received the final close ack, now it can leave
                    waitingForCloseAcks = false;
                }
                else
                {
                    if (m_Parameters.verbose > 3)
                    {
                        std::cout << "Rank " << m_RankMPI << " still need "
                                  << closeAcksNeeded.size() << " close acks [ ";
                        for (int n : closeAcksNeeded)
                        {
                            std::cout << " " << n;
                        }
                        std::cout << " ]" << std::endl;
                    }
                }
            }
        }
    }

    // Before leaving this method, subcoordinators need to update the variable tracking
    // the current file position for their particular subfile
    if (iAmSubCoord)
    {
        m_DataPos = currentFilePos;
    }

    if (m_Parameters.verbose > 3)
    {
        std::cout << "Rank " << m_RankMPI << " Exit ReroutingCommunicationLoop" << std::endl;
    }
}

void BP5Writer::WriteData_WithRerouting(format::BufferV *Data)
{
    m_ReadyToWrite = false;
    m_FinishedWriting = false;

    // Start the communication thread, all MPI communication happens over there
    std::thread commThread(&BP5Writer::ReroutingCommunicationLoop, this);

    if (m_Parameters.verbose > 3)
    {
        std::cout << "Background thread for rank " << m_RankMPI << " is now running" << std::endl;
    }

    // wait until communication thread indicates it's our turn to write
    {
        std::unique_lock<std::mutex> lck(m_WriteMutex);
        m_WriteCV.wait(lck, [this] { return m_ReadyToWrite; });
    }

    if (m_Parameters.verbose > 3)
    {
        std::cout << "Rank " << m_RankMPI << " signaled to write" << std::endl;
    }

    // Do the writing
    size_t substreamIdx = static_cast<size_t>(m_TargetIndex);

    // Check if we need to update which file we are writing to
    if (substreamIdx != m_Aggregator->m_SubStreamIndex)
    {
        // We were rerouted! Our aggregator subfile index is later exchanged with other
        // ranks to be written to metadata, so update it here or the metadata will be
        // wrong.
        m_Aggregator->m_SubStreamIndex = substreamIdx;

        // Open the subfile without doing any collective communications, since the global
        // coordinator ensures only one rerouted rank opens this file at a time. Also,
        // be sure to open in append mode because another rank already wrote to this file,
        // and open without append mode in that case can result in a block of zeros getting
        // written.
        OpenSubfile(false, true);
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

    // Wait for the communciation thread to return
    commThread.join();

    if (m_Parameters.verbose > 3)
    {
        std::cout << "Background thread for rank " << m_RankMPI << " is returning" << std::endl;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
