/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <adios2/helper/adiosCommMPI.h>
#include <adios2/helper/adiosRerouting.h>
#include <gtest/gtest.h>
#include <mpi.h>
#include <numeric>
#include <thread>

using namespace adios2;

namespace
{
int worldRank, worldSize;

void SendAndReceiveMessage(helper::Comm &comm, int destRank, int srcRank)
{
    std::cout << "Sending to " << destRank << " and expecting to receive from: " << srcRank << std::endl;

    // Send a message to another rank
    adios2::helper::RerouteMessage origMsg;
    origMsg.m_MsgType = adios2::helper::RerouteMessage::MessageType::WRITER_IDLE;
    origMsg.m_SrcRank = worldRank;
    origMsg.m_DestRank = destRank;
    origMsg.m_Offset = 2138;
    origMsg.m_Size = 1213;
    origMsg.SendTo(comm, destRank);

    // Receive a message from another (any) rank
    adios2::helper::RerouteMessage receivedMsg;
    receivedMsg.RecvFrom(comm, static_cast<int>(helper::Comm::Constants::CommRecvAny));

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


TEST_F(RerouteTest, TestMessageBuffer)
{
    adios2::helper::RerouteMessage origMsg;

    origMsg.m_MsgType = adios2::helper::RerouteMessage::MessageType::WRITER_IDLE;
    origMsg.m_SrcRank = 3;
    origMsg.m_DestRank = 0;
    origMsg.m_SubStreamIdx = 1;
    origMsg.m_Offset = 2138;
    origMsg.m_Size = 1213;

    std::vector<char> buf;
    origMsg.ToBuffer(buf);

    adios2::helper::RerouteMessage newMsg;
    newMsg.FromBuffer(buf);

    ASSERT_EQ(newMsg.m_MsgType, origMsg.m_MsgType);
    ASSERT_EQ(newMsg.m_SrcRank, origMsg.m_SrcRank);
    ASSERT_EQ(newMsg.m_DestRank, origMsg.m_DestRank);
    ASSERT_EQ(newMsg.m_SubStreamIdx, origMsg.m_SubStreamIdx);
    ASSERT_EQ(newMsg.m_Offset, origMsg.m_Offset);
    ASSERT_EQ(newMsg.m_Size, origMsg.m_Size);
}

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
