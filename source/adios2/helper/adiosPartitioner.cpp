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
#include <map>
#include <stdexcept>
#include <utility>
/// \endcond

namespace
{
    using Partitioner = std::function<adios2::helper::Partitioning(const std::vector<uint64_t>&, uint64_t)>;

    /**
     * Implements Greedy Number Partitioning as described here:
     *
     *     https://en.wikipedia.org/wiki/Greedy_number_partitioning
     *
     */
    adios2::helper::Partitioning PartitionGreedily(const std::vector<uint64_t> &values, uint64_t numberOfPartitions)
    {
        if (numberOfPartitions <= 0)
        {
            adios2::helper::Throw<std::runtime_error>("Helper", "adiosPartitioner", "PartitionGreedily",
                                                      "numberOfPartitions must be positive");
        }

        adios2::helper::Partitioning result;

        // Sort the incoming values, keeping track of the original indices
        std::vector<std::pair<int, int>> valuesAndIndices;
        for (int i = 0; i < values.size(); ++i) {
            valuesAndIndices.push_back(std::make_pair(values[i], i));
        }
        std::sort(valuesAndIndices.begin(),
                  valuesAndIndices.end(),
                  [](std::pair<int, int> a, std::pair<int, int> b)
                  {
                      return a.first > b.first;
                  });

        result.m_Partitions.resize(numberOfPartitions);
        result.m_Sizes.resize(numberOfPartitions);
        std::fill(result.m_Sizes.begin(), result.m_Sizes.end(), 0);

        for (int i = 0; i < valuesAndIndices.size(); ++i)
        {
            int index_of_smallest = std::distance(std::begin(result.m_Sizes),
                                                  std::min_element(std::begin(result.m_Sizes), std::end(result.m_Sizes)));
            result.m_Sizes[index_of_smallest] += valuesAndIndices[i].first;
            result.m_Partitions[index_of_smallest].push_back(valuesAndIndices[i].second);
        }

        return result;
    }

    /**
     * Registry of known partitioning strategies
     */
    std::map<adios2::helper::PartitioningStrategy, Partitioner> strategies = {
        {adios2::helper::PartitioningStrategy::GreedyNumberPartitioning, PartitionGreedily}
    };

    /**
     * Return the function implementing the given strategy, or raise an exception
     * if it cannot be found.
     */
    Partitioner GetPartitioner(adios2::helper::PartitioningStrategy strategy)
    {
        const auto it = strategies.find(strategy);
        if (it != strategies.end())
        {
            return it->second;
        }

        adios2::helper::Throw<std::runtime_error>("Helper", "adiosPartitioner", "GetPartitioner",
                                                  "unknown partitioning strategy");
    }
}

namespace adios2
{
namespace helper
{

std::pair<int, int> Partitioning::FindPartition(const int rank)
{
    std::pair<int, int> result(0, 0);
    for (int i = 0; i < m_Partitions.size(); ++i)
    {
        std::vector<int> nextPart = m_Partitions[i];
        const auto it = std::find(nextPart.begin(), nextPart.end(), rank);
        if (it != nextPart.end())
        {
            result.first = i;
            result.second = std::distance(std::begin(nextPart), it);
        }
    }
    return result;
}

Partitioning PartitionRanks(const std::vector<uint64_t> &rankValues,
                            uint64_t numberOfPartitions,
                            PartitioningStrategy strategy)
{
    Partitioner partitioner = GetPartitioner(strategy);
    return partitioner(rankValues, numberOfPartitions);
}

} // end namespace helper
} // end namespace adios2
