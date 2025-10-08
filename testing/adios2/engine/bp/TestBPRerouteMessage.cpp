/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <adios2/helper/adiosCommMPI.h>
#include "adios2/helper/adiosMemory.h"
#include <adios2/helper/adiosRerouting.h>
#include <gtest/gtest.h>
#include <mpi.h>
#include <numeric>
#include <sstream>
#include <thread>

using namespace adios2;

namespace
{
int worldRank, worldSize;

void SendAndReceiveMessage(helper::Comm &comm, int destRank, int srcRank)
{
    std::cout << "Sending to " << destRank << " and expecting to receive from: " << srcRank
              << std::endl;

    std::vector<char> sendBuffer;
    std::vector<char> recvBuffer;

    // Send a message to another rank
    adios2::helper::RerouteMessage origMsg;
    origMsg.m_MsgType = adios2::helper::RerouteMessage::MessageType::WRITER_IDLE;
    origMsg.m_SrcRank = worldRank;
    origMsg.m_DestRank = destRank;
    origMsg.m_Offset = 2138;
    origMsg.m_Size = 1213;
    origMsg.NonBlockingSendTo(comm, destRank, sendBuffer);

    int ready = 0;
    helper::Comm::Status status;

    while (!ready)
    {
        status = comm.Iprobe(static_cast<int>(helper::Comm::Constants::CommRecvAny), 0, &ready);
    }

    // Receive a message from another (any) rank
    adios2::helper::RerouteMessage receivedMsg;
    receivedMsg.BlockingRecvFrom(comm, status.Source, recvBuffer);

    std::stringstream ss;

    ss << "m_MsgType, orig = " << static_cast<int>(origMsg.m_MsgType)
              << ", rcvd = " << static_cast<int>(receivedMsg.m_MsgType) << "\n";
    ss << "m_SrcRank, orig = " << srcRank << ", rcvd = " << receivedMsg.m_SrcRank << "\n";
    ss << "m_DestRank, orig = " << worldRank << ", rcvd = " << receivedMsg.m_DestRank << "\n";
    ss << "m_Offset, orig = " << origMsg.m_Offset << ", rcvd = " << receivedMsg.m_Offset << "\n";
    ss << "m_Size, orig = " << origMsg.m_Size << ", rcvd = " << receivedMsg.m_Size << "\n";

    std::cout << ss.str();

    ASSERT_EQ(receivedMsg.m_MsgType, origMsg.m_MsgType);
    ASSERT_EQ(receivedMsg.m_SrcRank, srcRank);
    ASSERT_EQ(receivedMsg.m_DestRank, worldRank);
    ASSERT_EQ(receivedMsg.m_Offset, origMsg.m_Offset);
    ASSERT_EQ(receivedMsg.m_Size, origMsg.m_Size);
}
}

class RerouteTest : public ::testing::Test
{
public:
    RerouteTest() = default;
};

TEST_F(RerouteTest, TestSendReceiveRoundRobin)
{
    helper::Comm comm = adios2::helper::CommDupMPI(MPI_COMM_WORLD);

    int destRank = worldRank >= worldSize - 1 ? 0 : worldRank + 1;
    int expectedSender = worldRank <= 0 ? worldSize - 1 : worldRank - 1;

    SendAndReceiveMessage(comm, destRank, expectedSender);
}

TEST_F(RerouteTest, TestSendReceiveSelf)
{
    helper::Comm comm = adios2::helper::CommDupMPI(MPI_COMM_WORLD);

    int destRank = worldRank;
    int expectedSender = worldRank;

    SendAndReceiveMessage(comm, destRank, expectedSender);
}

TEST_F(RerouteTest, TestSendReceiveBare)
{
    helper::Comm comm = adios2::helper::CommDupMPI(MPI_COMM_WORLD);

    int sendToRank = worldRank >= worldSize - 1 ? 0 : worldRank + 1;
    int recvFromRank = worldRank <= 0 ? worldSize - 1 : worldRank - 1;

    std::vector<char> sendBuffer;

    sendBuffer.push_back(1);
    sendBuffer.push_back(2);
    sendBuffer.push_back(3);

    comm.Isend(sendBuffer.data(), sendBuffer.size(), sendToRank, 0);

    std::vector<char> recvBuffer;
    recvBuffer.resize(sendBuffer.size());

    comm.Recv(recvBuffer.data(), recvBuffer.size(), recvFromRank, 0);

    ASSERT_EQ(recvBuffer.size(), 3);
    ASSERT_EQ(static_cast<int>(recvBuffer[0]), 1);
    ASSERT_EQ(static_cast<int>(recvBuffer[1]), 2);
    ASSERT_EQ(static_cast<int>(recvBuffer[2]), 3);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
