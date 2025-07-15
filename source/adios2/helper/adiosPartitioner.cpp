/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPartitioner.cpp
 *
 *  Created on: May 13, 2025
 *      Author: Scott Wittenburg scott.wittenburg@kitware.com
 */

#include "adiosPartitioner.h"
#include "adiosLog.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
/// \endcond

namespace
{
using Partitioner =
    std::function<adios2::helper::Partitioning(const std::vector<uint64_t> &, uint64_t)>;

bool CompareLengths(std::vector<size_t> a, std::vector<size_t> b)
{
    if (a.size() < b.size())
    {
        return true;
    }
    return false;
}

/**
 * Implements Greedy Number Partitioning as described here:
 *
 *     https://en.wikipedia.org/wiki/Greedy_number_partitioning
 *
 */
adios2::helper::Partitioning PartitionGreedily(const std::vector<uint64_t> &values,
                                               uint64_t numberOfPartitions)
{
    if (numberOfPartitions <= 0)
    {
        adios2::helper::Throw<std::runtime_error>("Helper", "adiosPartitioner", "PartitionGreedily",
                                                  "numberOfPartitions must be positive");
    }

    adios2::helper::Partitioning result;

    // Sort the incoming values, keeping track of the original indices
    std::vector<std::pair<uint64_t, size_t>> valuesAndIndices;
    for (size_t i = 0; i < values.size(); ++i)
    {
        valuesAndIndices.push_back(std::make_pair(values[i], i));
    }
    std::sort(valuesAndIndices.begin(), valuesAndIndices.end(),
              [](std::pair<uint64_t, size_t> a, std::pair<uint64_t, size_t> b) {
                  return a.first > b.first;
              });

    result.m_Partitions.resize(numberOfPartitions);
    result.m_Sizes.resize(numberOfPartitions);
    std::fill(result.m_Sizes.begin(), result.m_Sizes.end(), 0);

    for (size_t i = 0; i < valuesAndIndices.size(); ++i)
    {
        auto dataSize = valuesAndIndices[i].first;
        int64_t index_of_smallest;
        if (dataSize <= 0)
        {
            // If rank has no data, it can be added to the shortest chain without
            // regard for the sum of data already present in each partition, and then
            // we also don't need to add the size of the rank data to the partition total
            index_of_smallest =
                std::distance(std::begin(result.m_Partitions),
                              std::min_element(std::begin(result.m_Partitions),
                                               std::end(result.m_Partitions), CompareLengths));
        }
        else
        {
            // If the rank has any data, add it to the partition with the smallest total size
            // and update the total size of that partition
            index_of_smallest = std::distance(
                std::begin(result.m_Sizes),
                std::min_element(std::begin(result.m_Sizes), std::end(result.m_Sizes)));
            result.m_Sizes[index_of_smallest] += dataSize;
        }

        result.m_Partitions[index_of_smallest].push_back(valuesAndIndices[i].second);
    }

    return result;
}

/**
 * Registry of known partitioning strategies
 */
std::map<adios2::helper::PartitioningStrategy, Partitioner> strategies = {
    {adios2::helper::PartitioningStrategy::GreedyNumberPartitioning, PartitionGreedily}};

/**
 * Return the function implementing the given strategy, or raise an exception
 * if it cannot be found.
 */
Partitioner GetPartitioner(adios2::helper::PartitioningStrategy strategy)
{
    const auto it = strategies.find(strategy);
    if (it == strategies.end())
    {
        adios2::helper::Throw<std::runtime_error>("Helper", "adiosPartitioner", "GetPartitioner",
                                                  "unknown partitioning strategy");
    }

    return it->second;
}
}

namespace adios2
{
namespace helper
{

RankPartition Partitioning::FindPartition(const int parentRank)
{
    RankPartition result;
    for (size_t i = 0; i < m_Partitions.size(); ++i)
    {
        std::vector<size_t> nextPart = m_Partitions[i];
        const auto it = std::find(nextPart.begin(), nextPart.end(), parentRank);
        if (it != nextPart.end())
        {
            result.m_subStreamIndex = i;
            result.m_aggregatorRank = static_cast<int>(*(nextPart.begin()));
            result.m_rankOrder = static_cast<int>(std::distance(std::begin(nextPart), it));
        }
    }
    result.m_subStreams = static_cast<int>(m_Partitions.size());
    return result;
}

void Partitioning::PrintSummary()
{
    std::cout << "Paritioning resulted in " << m_Partitions.size() << " substreams:" << std::endl;
    for (size_t i = 0; i < m_Partitions.size(); ++i)
    {
        std::vector<size_t> nextPart = m_Partitions[i];
        std::cout << "  " << i << ": [";
        for (size_t j = 0; j < nextPart.size(); ++j)
        {
            std::cout << nextPart[j] << " ";
        }
        std::cout << "], partition data size: " << m_Sizes[i] << std::endl;
    }
}

Partitioning PartitionRanks(const std::vector<uint64_t> &rankValues, uint64_t numberOfPartitions,
                            PartitioningStrategy strategy)
{
    Partitioner partitioner = GetPartitioner(strategy);
    return partitioner(rankValues, numberOfPartitions);
}

} // end namespace helper
} // end namespace adios2
