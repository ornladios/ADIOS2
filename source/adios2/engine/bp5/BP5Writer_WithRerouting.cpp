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

    int subCoord = m_Aggregator->m_AggregatorRank;
    bool iAmSubCoord = m_RankMPI == subCoord;

    // Arbitrarily decide that the sub coordinator of the first partition is also
    // the global coordinator
    int globalCoord = m_Partitioning.m_Partitions[0][0];
    bool iAmGlobalCoord = m_RankMPI == globalCoord;

    // Some containers only used by augmented roles:

    // sub coordinators maintain a queue of writers
    std::queue<int> writerQueue;

    // global coordinator keeps track of queue sizes of each subcoord
    std::vector<int> queueSizes;
    std::vector<int> subCoordRanks;

    // Sends are non-blocking. We use the pool to avoid the situation where the
    // buffer is destructed before the send is complete.  If we start seeing
    // many Sends pending for a long time, that could cause us to exhaust the
    // pool of buffers, at which point we would start overwriting buffers in the
    // pool, potentially creating errors which are difficult to debug/diagnose.
    BufferPool sendBuffers((iAmSubCoord ? 100 : 1));
    std::vector<char> recvBuffer;
    int writingRank = -1;
    uint64_t currentFilePos = 0;
    bool sentFinished = false;
    bool commThreadFinished = false;
    bool firstIdleMsg = true;

    if (iAmGlobalCoord)
    {
        queueSizes.resize(m_CommAggregators.Size());
        subCoordRanks.resize(m_CommAggregators.Size());

        for (size_t i = 0; i < m_Partitioning.m_Partitions.size(); ++i)
        {
            queueSizes[i] = -1; // indicates we don't know the size of that queue
            subCoordRanks[i] = m_Partitioning.m_Partitions[i][0];
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

    while (!commThreadFinished)
    {
        int msgReady = 0;
        helper::Comm::Status status =
            m_Comm.Iprobe(static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &msgReady);

        // If there is a message ready, receive and handle it
        if (msgReady)
        {
            RerouteMessage message;
            message.BlockingRecvFrom(m_Comm, status.Source, recvBuffer);

            switch ((RerouteMessage::MessageType)message.m_MsgType)
            {
            case RerouteMessage::MessageType::DO_WRITE:
                {
                    std::unique_lock<std::mutex> lck(m_WriteMutex);
                    m_TargetIndex = message.m_SubStreamIdx;
                    m_DataPos = message.m_Offset;
                    m_TargetCoordinator = message.m_SrcRank;
                    m_ReadyToWrite = true;
                    m_WriteCV.notify_one();
                }
                break;
            case RerouteMessage::MessageType::WRITE_COMPLETION:
                currentFilePos = message.m_Offset;
                writingRank = -1;
                break;
            case RerouteMessage::MessageType::GROUP_CLOSE:
                m_DataPos = currentFilePos;
                commThreadFinished = true;
                continue;
            case RerouteMessage::MessageType::WRITER_IDLE:
                {
                    int idleWriter = message.m_SrcRank;
                    size_t idleGroup = static_cast<size_t>(message.m_SubStreamIdx);
                    queueSizes[idleGroup] = 0;

                    if (firstIdleMsg)
                    {
                        for (size_t i = 0; i < subCoordRanks.size(); ++i)
                        {
                            int scRank = subCoordRanks[i];
                            // No need to ask the sender of the WRITER_IDLE msg how big their queue is
                            if (scRank != idleWriter)
                            {
                                adios2::helper::RerouteMessage inquiryMsg;
                                inquiryMsg.m_MsgType = RerouteMessage::MessageType::STATUS_INQUIRY;
                                inquiryMsg.m_SrcRank = m_RankMPI;
                                inquiryMsg.m_DestRank = scRank;
                                inquiryMsg.NonBlockingSendTo(m_Comm, scRank, sendBuffers.GetNextBuffer());
                            }
                        }

                        firstIdleMsg = false;
                    }
                }
                break;
            case RerouteMessage::MessageType::STATUS_INQUIRY:
                adios2::helper::RerouteMessage replyMsg;
                replyMsg.m_MsgType = RerouteMessage::MessageType::STATUS_REPLY;
                replyMsg.m_SrcRank = m_RankMPI;
                replyMsg.m_DestRank = globalCoord;
                replyMsg.m_SubStreamIdx = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                replyMsg.m_Size = static_cast<uint64_t>(writerQueue.size());
                replyMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
                break;
            case RerouteMessage::MessageType::STATUS_REPLY:
                {
                    size_t idx = static_cast<size_t>(message.m_SubStreamIdx);
                    queueSizes[idx] = static_cast<int>(message.m_Size);
                }
                break;
            default:
                break;
            }
        }

        // Check if writing has finished, and alert the target SC
        {
            std::lock_guard<std::mutex> lck(m_NotifMutex);
            if (m_FinishedWriting && !sentFinished)
            {
                adios2::helper::RerouteMessage writeCompleteMsg;
                writeCompleteMsg.m_MsgType = RerouteMessage::MessageType::WRITE_COMPLETION;
                writeCompleteMsg.m_SrcRank = m_RankMPI;
                writeCompleteMsg.m_DestRank = m_TargetCoordinator;
                writeCompleteMsg.m_SubStreamIdx = m_TargetIndex;
                writeCompleteMsg.m_Offset = m_DataPos;
                writeCompleteMsg.NonBlockingSendTo(m_Comm, m_TargetCoordinator,
                                                   sendBuffers.GetNextBuffer());
                sentFinished = true;

                if (!iAmSubCoord && !iAmGlobalCoord)
                {
                    // My only role was to write (no communication responsibility) so I am
                    // done at this point.
                    commThreadFinished = true;
                    continue;
                }
            }
        }

        // Check if anyone is writing right now, and if not, ask the next writer to start
        if (iAmSubCoord && writingRank == -1)
        {
            if (!writerQueue.empty())
            {
                // Pop the queue and send DO_WRITE
                int nextWriter = writerQueue.front();
                writerQueue.pop();
                adios2::helper::RerouteMessage writeMsg;
                writeMsg.m_MsgType = RerouteMessage::MessageType::DO_WRITE;
                writeMsg.m_SrcRank = m_RankMPI;
                writeMsg.m_DestRank = nextWriter;
                writeMsg.m_SubStreamIdx = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                writeMsg.m_Offset = currentFilePos;
                writingRank = nextWriter;
                writeMsg.NonBlockingSendTo(m_Comm, nextWriter, sendBuffers.GetNextBuffer());
            }
            else
            {
                // Writer queue now empty, send WRITE_IDLE to the GC
                // TODO: If this group is already over the threshold ratio, send
                // TODO: GROUP_CLOSE instead of WRITER_IDLE
                adios2::helper::RerouteMessage idleMsg;
                idleMsg.m_MsgType = RerouteMessage::MessageType::WRITER_IDLE;
                idleMsg.m_SrcRank = m_RankMPI;
                idleMsg.m_DestRank = globalCoord;
                idleMsg.m_SubStreamIdx = static_cast<int>(m_Aggregator->m_SubStreamIndex);
                idleMsg.NonBlockingSendTo(m_Comm, globalCoord, sendBuffers.GetNextBuffer());
            }
        }
    }
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

    // std::cout << "Background thread for rank " << m_RankMPI << " is now running" << std::endl;

    // wait until communication thread indicates it's our turn to write
    {
        std::unique_lock<std::mutex> lck(m_WriteMutex);
        m_WriteCV.wait(lck, [this] { return m_ReadyToWrite; });
    }

    // Do the writing

    // align to PAGE_SIZE
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos, m_Parameters.StripeSize);
    m_StartDataPos = m_DataPos;

    std::vector<core::iovec> DataVec = Data->DataVec();
    // TODO: use m_TargetIndex here instead of being locked into aggregator's index
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
