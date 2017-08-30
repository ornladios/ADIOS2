/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Aggregator.cpp :
 *
 *  Created on: Mar 21, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP1Aggregator.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <numeric> //std::accumulate
#include <unordered_set>
/// \endcond

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

BP1Aggregator::BP1Aggregator(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
}

std::vector<char>
BP1Aggregator::SetCollectiveProfilingJSON(const std::string &rankLog) const
{
    // Gather sizes
    const size_t rankLogSize = rankLog.size();
    std::vector<size_t> rankLogsSizes = GatherValues(rankLogSize, m_MPIComm);

    // Gatherv JSON per rank
    std::vector<char> profilingJSON(3);
    const std::string header("[\n");
    const std::string footer("\n]\n");
    size_t gatheredSize = 0;
    size_t position = 0;

    if (m_RankMPI == 0) // pre-allocate in destination
    {
        gatheredSize =
            std::accumulate(rankLogsSizes.begin(), rankLogsSizes.end(), 0);

        profilingJSON.resize(gatheredSize + header.size() + footer.size() - 2);
        CopyToBuffer(profilingJSON, position, header.c_str(), header.size());
    }

    GathervArrays(rankLog.c_str(), rankLog.size(), rankLogsSizes.data(),
                  rankLogsSizes.size(), &profilingJSON[position], m_MPIComm);

    if (m_RankMPI == 0) // add footer to close JSON
    {
        position += gatheredSize - 2;
        CopyToBuffer(profilingJSON, position, footer.c_str(), footer.size());
    }

    return profilingJSON;
}

void BP1Aggregator::GathervBuffers(const std::vector<char> &bufferIn,
                                   std::vector<char> &bufferOut,
                                   size_t &position) const
{
    GathervVectors(bufferIn, bufferOut, position, m_MPIComm);
}

} // end namespace format
} // end namespace adios2
