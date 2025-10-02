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
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include <adios2-perfstubs-interface.h>

#include <algorithm> // max
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;
using namespace adios2::helper;

void BP5Writer::ReroutingCommunicationLoop()
{
    // Sleep for a random amount of time between 0 and 6 seconds
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 6000);
    int sleepMillis = distrib(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMillis));

    int subCoord = m_Aggregator->m_AggregatorRank;
    std::queue<int> writerQueue;
    int writingRank = -1;
    uint64_t currentFilePos;

    // Now start the
    std::cout << "    Rank " << m_Comm.Rank() << " ReroutingCommunicationLoop()" << std::endl;
    std::cout << "        SC: " << subCoord << std::endl;
    std::cout << "        GC: " << m_Comm.Size() - 1 << std::endl;
    std::cout << "        subfile index: " << m_Aggregator->m_SubStreamIndex << std::endl;
    std::cout << "        total subfiles: " << m_Aggregator->m_SubStreams << std::endl;

    if (m_Rank == subCoord && m_DataPosShared == true)
    {
        // We are a subcoordinator and have shared data pos after a previous timestep,
        // we should update our notion of m_DataPos
        currentFilePos = m_SubstreamDataPos[a->m_SubStreamIndex];
        m_DataPosShared = false;
    }

    // align to PAGE_SIZE
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos, m_Parameters.StripeSize);
    m_StartDataPos = m_DataPos;

    // First send a message to the SC to get added to their writing queue
    adios2::helper::RerouteMessage submitMsg;
    submitMsg.m_MsgType = RerouteMessage::MessageType::WRITE_SUBMISSION;
    submitMsg.m_SrcRank = m_RankMPI;
    submitMsg.m_DestRank = subCoord;
    submitMsg.SendTo(m_Comm, subCoord);

    while (true)
    {
        int msgReady = 0;

        m_Comm.Iprobe(static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &msgReady);

        if (msgReady)
        {
            // If there is a message ready, receive and handle it
            adios2::helper::RerouteMessage message;
            message.RecvFrom(m_Comm, static_cast<int>(helper::Comm::Constants::CommRecvAny));

            switch (message.m_MsgType)
            {
            case (int)RerouteMessage::MessageType::WRITE_SUBMISSION:
                writerQueue.push(message.m_SrcRank);
                break;
            case (int)RerouteMessage::MessageType::DO_WRITE:
                m_TargetIndex = message.m_SubStreamIdx;
                m_DataPos = message.m_Offset;
                break;
            default:
                break;
            }
        }

        // Check if writing has finished, and alert the target SC

        // Check if anyone is writing right now, and if not, ask the next writer to start
        if (writingRank == -1)
        {
            if (!writerQueue.empty())
            {
                int nextWriter = writerQueue.pop();
                adios2::helper::RerouteMessage writeMsg;
                writeMsg.m_MsgType = adios2::helper::RerouteMessage::MessageType::DO_WRITE;
                writeMsg.m_SrcRank = m_RankMPI;
                writeMsg.m_DestRank = nextWriter;
                writeMsg.m_SubStreamIdx = m_Aggregator->m_SubStreamIndex;
                writeMsg.m_Offset = currentFilePos;
                writeMsg.SendTo(comm, nextWriter);
                writingRank = nextWriter;
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

    m_readyToWrite = false;

    std::thread commThread(&BP5Writer::ReroutingCommunicationLoop, this);

    std::cout << "Background thread for rank " << m_RankMPI << " is now running" << std::endl;

    commThread.join();

    std::cout << "Background thread for rank " << m_RankMPI << " is now finished" << std::endl;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
