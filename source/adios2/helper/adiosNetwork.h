/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosNetwork.h
 * Common Network functions needed by staging and streaming engines
 *
 *  Created on: March 22, 2019
 *      Author: Jason Wang
 */

#ifndef ADIOS2_HELPER_ADIOSNETWORK_H_
#define ADIOS2_HELPER_ADIOSNETWORK_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <mpi.h>
#include <string>
#include <vector>
/// \endcond

namespace adios2
{
namespace helper
{

#ifndef _WIN32
/**
 * returns a vector of strings with all available IP addresses on the node
 * @return vector of strings
 */
std::vector<std::string> AvailableIpAddresses() noexcept;

void HandshakeWriter(MPI_Comm mpiComm, size_t &appID,
                     std::vector<std::string> &fullAddresses,
                     const std::string name, const int basePort,
                     const int channelsPerRank, const int maxRanksPerNode = 100,
                     const int maxAppsPerNode = 10);

void HandshakeReader(MPI_Comm mpiComm, size_t &appID,
                     std::vector<std::string> &fullAddresses,
                     const std::string name);

#endif

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSNETWORK_H_ */
