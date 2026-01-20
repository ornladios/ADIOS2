/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include <adios2.h>
#include <adios2/helper/adiosCommMPI.h>
#include <gtest/gtest.h>
#include <mpi.h>

using namespace adios2;

namespace
{
int worldRank, worldSize;
}

class CharBufferMPITest : public ::testing::Test
{
public:
    CharBufferMPITest() = default;
};

TEST_F(CharBufferMPITest, TestSendReceiveCharBuffer)
{
    helper::Comm comm = adios2::helper::CommWithMPI(MPI_COMM_WORLD);

    int sendToRank = worldRank >= worldSize - 1 ? 0 : worldRank + 1;
    int recvFromRank = worldRank <= 0 ? worldSize - 1 : worldRank - 1;

    std::vector<char> sendBuffer = {1, 2, 3};

    comm.Isend(sendBuffer.data(), sendBuffer.size(), sendToRank, 0);

    std::vector<char> recvBuffer;
    recvBuffer.resize(sendBuffer.size());

    comm.Recv(recvBuffer.data(), recvBuffer.size(), recvFromRank, 0);

    ASSERT_EQ(recvBuffer.size(), sendBuffer.size());

    for (size_t i = 0; i < recvBuffer.size(); ++i)
    {
        ASSERT_EQ(recvBuffer[i], sendBuffer[i]);
    }
}

TEST_F(CharBufferMPITest, TestSendReceiveCharBufferNoHelper)
{
    int sendToRank = worldRank >= worldSize - 1 ? 0 : worldRank + 1;
    int recvFromRank = worldRank <= 0 ? worldSize - 1 : worldRank - 1;
    constexpr int count = 3;

    // Send the buffer of chars (non-blocking)
    MPI_Request request = MPI_REQUEST_NULL;
    char sendBuffer[count] = {1, 2, 3};
    MPI_Isend(&sendBuffer, count, MPI_CHAR, sendToRank, 0, MPI_COMM_WORLD, &request);

    // Receive the buffer of chars (blocking)
    MPI_Status status;
    char recvBuffer[count];
    int ret = MPI_Recv(&recvBuffer, count, MPI_CHAR, recvFromRank, 0, MPI_COMM_WORLD, &status);

    ASSERT_EQ(ret, MPI_SUCCESS);

    int mpiCount = 0;
    ret = MPI_Get_count(&status, MPI_CHAR, &mpiCount);

    ASSERT_EQ(ret, MPI_SUCCESS);
    ASSERT_EQ(mpiCount, count);

    for (int i = 0; i < count; ++i)
    {
        ASSERT_EQ(recvBuffer[i], sendBuffer[i]);
    }
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
