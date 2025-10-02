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
#include "adios2/helper/adiosRerouting.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include <adios2-perfstubs-interface.h>

#include <ctime>
#include <iostream>
#include <thread>

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

void BP5Writer::ReroutingCommunicationLoop()
{
    using RerouteMessage = typename adios2::helper::RerouteMessage;

    int subCoord = m_Aggregator->m_AggregatorRank;
    bool iAmSubCoord = m_RankMPI == subCoord;
    // Arbitrarily make the last rank the global coordinator
    // TODO: should the global coordinator role be assigned to a subcoordinator?
    bool iAmGlobalCoord = m_RankMPI == m_Comm.Size() - 1;
    std::queue<int> writerQueue;
    int writingRank = -1;
    uint64_t currentFilePos = 0;
    bool sentFinished = false;

    // Now start the
    std::cout << "    Rank " << m_Comm.Rank() << " ReroutingCommunicationLoop()" << std::endl;
    std::cout << "        SC: " << subCoord << std::endl;
    std::cout << "        GC: " << m_Comm.Size() - 1 << std::endl;
    std::cout << "        subfile index: " << m_Aggregator->m_SubStreamIndex << std::endl;
    std::cout << "        total subfiles: " << m_Aggregator->m_SubStreams << std::endl;

    if (iAmSubCoord && m_DataPosShared == true)
    {
        // We are a subcoordinator and have shared data pos after a previous timestep,
        // we should update our notion of m_DataPos
        currentFilePos = m_SubstreamDataPos[m_Aggregator->m_SubStreamIndex];
        m_DataPosShared = false;
    }

    // First send a message to the SC to get added to their writing queue
    RerouteMessage submitMsg;
    submitMsg.m_MsgType = RerouteMessage::MessageType::WRITE_SUBMISSION;
    submitMsg.m_SrcRank = m_RankMPI;
    submitMsg.m_DestRank = subCoord;
    submitMsg.SendTo(m_Comm, subCoord);

    while (true)
    {
        int msgReady = 0;
        helper::Comm::Status status = m_Comm.Iprobe(
            static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &msgReady);

        // If there is a message ready, receive and handle it
        // if (msgReady)
        while (msgReady)
        {
            RerouteMessage message;
            message.RecvFrom(m_Comm, status.Source);

            switch ((RerouteMessage::MessageType) message.m_MsgType)
            {
            case RerouteMessage::MessageType::WRITE_SUBMISSION:
                writerQueue.push(message.m_SrcRank);
                break;
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
            default:
                break;
            }

            msgReady = 0;
            status = m_Comm.Iprobe(static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &msgReady);
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
                writeCompleteMsg.SendTo(m_Comm, m_TargetCoordinator);
                sentFinished = true;

                if (!iAmSubCoord /*&& !iAmGlobalCoord*/ )
                {
                    // My only role was to write (no communication responsibility) so I am
                    // done at this point.
                    break;
                }
            }
        }

        // Check if anyone is writing right now, and if not, ask the next writer to start
        if (iAmSubCoord && writingRank == -1)
        {
            if (!writerQueue.empty())
            {
                int nextWriter = writerQueue.front();
                writerQueue.pop();
                adios2::helper::RerouteMessage writeMsg;
                writeMsg.m_MsgType = RerouteMessage::MessageType::DO_WRITE;
                writeMsg.m_SrcRank = m_RankMPI;
                writeMsg.m_DestRank = nextWriter;
                writeMsg.m_SubStreamIdx = m_Aggregator->m_SubStreamIndex;
                writeMsg.m_Offset = currentFilePos;
                writingRank = nextWriter;
                writeMsg.SendTo(m_Comm, nextWriter);
            }
            else
            {
                // TODO: Send WRITE_IDLE to the GC instead of ending the loop here
                break;
            }
        }
    }
}

void BP5Writer::WriteData_WithRerouting(format::BufferV *Data)
{
    // - start the communcation loop running in a thread
    // - begin polling the write barrier variable, once it flips. write:
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
        m_WriteCV.wait(lck, [this]{ return m_ReadyToWrite; });
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

    std::cout << "Background thread for rank " << m_RankMPI << " is now finished" << std::endl;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
