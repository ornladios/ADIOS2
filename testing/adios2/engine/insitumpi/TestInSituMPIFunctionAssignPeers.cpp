/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <vector>

//#include <adios2.h>
#include "adios2/engine/insitumpi/InSituMPIFunctions.h"

#include <gtest/gtest.h>

#ifdef ADIOS2_HAVE_MPI
#include "mpi.h"
#endif

class InSituMPIFunctionAssignPeersTest : public ::testing::Test
{
public:
    InSituMPIFunctionAssignPeersTest() = default;
};

//******************************************************************************
// N writers M readers, N > M, test on reader side
//******************************************************************************

// 3 by 2
TEST_F(InSituMPIFunctionAssignPeersTest, 3x2)
{
    // 3 writers 2 readers
    // expect 2 writers assigned to first reader (rank 0 peers = 0,1)
    // expect the last writer assigned to second reader (rank 1 peers = 2)
    std::vector<int> RankWriters({0, 1, 2});
    int mProc = 2;
    std::vector<int> myPeers;
    int myRank;

    myRank = 0;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 2);
    ASSERT_EQ(myPeers.at(0), 0);
    ASSERT_EQ(myPeers.at(1), 1);

    myRank = 1;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 2);
}

// 5 by 2
TEST_F(InSituMPIFunctionAssignPeersTest, 5x2)
{
    // 5 writers 2 readers
    // expect 3 writers assigned to first reader (rank 0 peers = 0,1,2)
    // expect 2 writers assigned to second reader (rank 1 peers = 3,4)
    std::vector<int> RankWriters({0, 1, 2, 3, 4});
    int mProc = 2;
    std::vector<int> myPeers;
    int myRank;

    myRank = 0;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 3);
    ASSERT_EQ(myPeers.at(0), 0);
    ASSERT_EQ(myPeers.at(1), 1);
    ASSERT_EQ(myPeers.at(2), 2);

    myRank = 1;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 2);
    ASSERT_EQ(myPeers.at(0), 3);
    ASSERT_EQ(myPeers.at(1), 4);
}

// 8 by 3
TEST_F(InSituMPIFunctionAssignPeersTest, 8x3)
{
    // 8 writers 3 readers
    // expect 3 writers assigned to first reader (rank 0 peers = 0,1,2)
    // expect 3 writers assigned to second reader (rank 1 peers = 3,4,5)
    // expect 2 writers assigned to third reader (rank 1 peers = 6,7)
    std::vector<int> RankWriters({0, 1, 2, 3, 4, 5, 6, 7});
    int mProc = 3;
    std::vector<int> myPeers;
    int myRank;

    myRank = 0;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 3);
    ASSERT_EQ(myPeers.at(0), 0);
    ASSERT_EQ(myPeers.at(1), 1);
    ASSERT_EQ(myPeers.at(2), 2);

    myRank = 1;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 3);
    ASSERT_EQ(myPeers.at(0), 3);
    ASSERT_EQ(myPeers.at(1), 4);
    ASSERT_EQ(myPeers.at(2), 5);

    myRank = 2;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 2);
    ASSERT_EQ(myPeers.at(0), 6);
    ASSERT_EQ(myPeers.at(1), 7);
}

//******************************************************************************
// N writers M readers, N = M, test on reader side
//******************************************************************************

// 3 by 3
TEST_F(InSituMPIFunctionAssignPeersTest, 3x3)
{
    // 3 writers 3 readers
    // expect 1 writer assigned to each reader (rank K peer = K)
    std::vector<int> RankWriters({0, 1, 2});
    int mProc = 3;
    std::vector<int> myPeers;
    int myRank;

    for (int i = 0; i < mProc; i++)
    {
        myRank = 0;
        myPeers.clear();
        myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
        ASSERT_EQ(myPeers.size(), 1);
        ASSERT_EQ(myPeers.at(0), myRank);
    }
}

//******************************************************************************
// N writers M readers, N < M, test on reader side
//******************************************************************************

// 2 by 3
TEST_F(InSituMPIFunctionAssignPeersTest, 2x3)
{
    // 2 writers 3 readers
    // expect 1 writer assigned to each reader
    // same writer assigned to rank 0 and 1, the other writer to the rank 1
    std::vector<int> RankWriters({0, 1});
    int mProc = 3;
    std::vector<int> myPeers;
    int myRank;

    myRank = 0;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 1;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 2;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 1);
}

// 3 by 8
TEST_F(InSituMPIFunctionAssignPeersTest, 3x8)
{
    // 3 writers 8 readers
    // expect 1 writer assigned to each reader
    // writer 0 assigned to readers 0..2 (3 peers)
    // writer 1 assigned to readers 3..5 (3 peers)
    // writer 2 assigned to readers 6..7 (2 peers)
    std::vector<int> RankWriters({0, 1, 2});
    int mProc = 8;
    std::vector<int> myPeers;
    int myRank;

    myRank = 0;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 1;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 2;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 3;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 1);

    myRank = 4;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 1);

    myRank = 5;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 1);

    myRank = 6;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 2);

    myRank = 7;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 2);
}

// 3 by 13
TEST_F(InSituMPIFunctionAssignPeersTest, 3x13)
{
    // 3 writers 13 readers
    // expect 1 writer assigned to each reader
    // writer 0 assigned to readers 0..4 (5 peers)
    // writer 1 assigned to readers 5..8 (4 peers)
    // writer 2 assigned to readers 9..12 (4 peers)
    std::vector<int> RankWriters({0, 1, 2});
    int mProc = 13;
    std::vector<int> myPeers;
    int myRank;

    myRank = 0;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 4;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 0);

    myRank = 5;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 1);

    myRank = 8;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 1);

    myRank = 9;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 2);

    myRank = 12;
    myPeers.clear();
    myPeers = adios2::insitumpi::AssignPeers(myRank, mProc, RankWriters);
    ASSERT_EQ(myPeers.size(), 1);
    ASSERT_EQ(myPeers.at(0), 2);
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
    MPI_Init(nullptr, nullptr);

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    MPI_Finalize();
    return result;
}
