/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cmath>
#include <cstdint>
#include <cstring>

#include <iostream>
#include <limits>
#include <stdexcept>

#include <adios2.h>
#include <adios2/common/ADIOSTypes.h>
#include <adios2/helper/adiosPartitioner.h>

#include <gtest/gtest.h>

TEST(ADIOS2Partitioner, ADIOS2PartitionerGreedy)
{
    std::vector<uint64_t> dataSizes = {97, 38, 10, 98, 2, 27, 40, 0, 54, 64, 88, 2};
    uint64_t numPartitions = 4;
    adios2::helper::Partitioning result = adios2::helper::PartitionRanks(
        dataSizes, numPartitions, adios2::helper::PartitioningStrategy::GreedyNumberPartitioning);

    // expected sizes: [129, 135, 128, 128]
    ASSERT_EQ(result.m_Sizes.size(), numPartitions);
    ASSERT_EQ(result.m_Sizes[0], 129);
    ASSERT_EQ(result.m_Sizes[1], 135);
    ASSERT_EQ(result.m_Sizes[2], 128);
    ASSERT_EQ(result.m_Sizes[3], 128);

    // expected partitions: [[3, 5, 4, 11], [0, 1], [10, 6, 7], [9, 8, 2]]
    ASSERT_EQ(result.m_Partitions.size(), numPartitions);
    ASSERT_EQ(result.m_Partitions[0].size(), 4);
    ASSERT_EQ(result.m_Partitions[0][0], 3);
    ASSERT_EQ(result.m_Partitions[0][1], 5);
    ASSERT_EQ(result.m_Partitions[0][2], 4);
    ASSERT_EQ(result.m_Partitions[0][3], 11);
    ASSERT_EQ(result.m_Partitions[1].size(), 3);
    ASSERT_EQ(result.m_Partitions[1][0], 0);
    ASSERT_EQ(result.m_Partitions[1][1], 1);
    ASSERT_EQ(result.m_Partitions[1][2], 7);
    ASSERT_EQ(result.m_Partitions[2].size(), 2);
    ASSERT_EQ(result.m_Partitions[2][0], 10);
    ASSERT_EQ(result.m_Partitions[2][1], 6);
    ASSERT_EQ(result.m_Partitions[3].size(), 3);
    ASSERT_EQ(result.m_Partitions[3][0], 9);
    ASSERT_EQ(result.m_Partitions[3][1], 8);
    ASSERT_EQ(result.m_Partitions[3][2], 2);

    dataSizes = {10, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0, 0};
    numPartitions = 4;
    result = adios2::helper::PartitionRanks(
        dataSizes, numPartitions, adios2::helper::PartitioningStrategy::GreedyNumberPartitioning);

    // expected partitions: [[0, 4, 8], [1, 5, 9], [2, 6, 10], [3, 7, 11]]
    ASSERT_EQ(result.m_Partitions.size(), numPartitions);
    ASSERT_EQ(result.m_Sizes.size(), numPartitions);

    for (size_t i = 0; i < result.m_Partitions.size(); ++i)
    {
        ASSERT_EQ(result.m_Partitions[i].size(), 3);
        ASSERT_EQ(result.m_Sizes[i], 10);
    }
}

int main(int argc, char **argv)
{
    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
