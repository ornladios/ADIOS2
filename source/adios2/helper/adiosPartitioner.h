/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPartitioner.h helper functions for data-size based aggregation partitioning
 *
 *  Created on: May 13, 2025
 *      Author: Scott Wittenburg scott.wittenburg@kitware.com
 */

#ifndef ADIOS2_HELPER_ADIOSPARTITIONER_H_
#define ADIOS2_HELPER_ADIOSPARTITIONER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstddef>
#include <cstdint>
#include <vector>
/// \endcond

namespace adios2
{

namespace helper
{

struct RankPartition
{
    // These are typed to matched how they're used in MPIAggregator
    size_t m_subStreamIndex;
    int m_aggregatorRank;
    int m_rankOrder;
    int m_subStreams;
};

/**
 * Represent the results of a partitioning
 */
struct Partitioning
{
    // List of partitions, each partition is a list of rank ids
    std::vector<std::vector<size_t>> m_Partitions;
    // List of partition sizes (sum of sizes of component ranks)
    std::vector<uint64_t> m_Sizes;

    // Given a rank id, return its partition index index within the partition
    RankPartition FindPartition(int rank);
    void PrintSummary();
};

/**
 * Enumerated partitioning strategies
 */
enum class PartitioningStrategy
{
    GreedyNumberPartitioning,
    Auto
};

/**
 * Invokes a particular strategy to partition ranks based on provided values.
 * Pass a positive integer to specify the desired number of partitions
 * explicitly, or pass -1 to let the partitioning strategy choose the
 * number of partitions.
 *
 * If the selected strategy does not exist or does not support choosing
 * a number of partitions when -1 is passed, an exception is raised.
 */
Partitioning
PartitionRanks(const std::vector<uint64_t> &rankValues, uint64_t numberOfPartitions = -1,
               PartitioningStrategy strategy = PartitioningStrategy::GreedyNumberPartitioning);

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSPARTITIONER_H_ */
