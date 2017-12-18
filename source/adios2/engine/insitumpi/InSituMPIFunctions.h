/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIFunctions.h
 * common functions for in situ MPI Writer and Reader
 * It requires MPI
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */

#ifndef ADIOS2_ENGINE_INSITUMPIFunctions_H_
#define ADIOS2_ENGINE_INSITUMPIFunctions_H_

#include <mpi.h>

#include <string>
#include <vector>

namespace adios2
{

namespace insitumpi
{

// Generate the list of the other processes in MPI_COMM_WORLD who are
// the partner in Write--Read. comm is 'our' communicator.
// name is the output/input name the Open() was called with
std::vector<int> FindPeers(MPI_Comm comm, std::string name, bool amIWriter);

} // end namespace insitumpi

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIFUNCTIONS_H_ */
