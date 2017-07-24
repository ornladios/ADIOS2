/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.h : collection of MPI functions used across adios2
 *
 *  Created on: Jul 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOMPIFUNCTIONS_H_
#define ADIOS2_HELPER_ADIOMPIFUNCTIONS_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "adios2/ADIOSMPICommOnly.h"

namespace adios2
{

/**
 * rankSource: owns a string and broadcast to all other ranks
 * Others: receive std::vector<char> and copy to a string
 * @param mpiComm MPI communicator defining all ranks and size domain
 * @param input string input from rankSource
 * @param rankSource rank that broadcast the string, (default = 0)
 * @return input contents for each rank
 */
std::string BroadcastString(const std::string &input, MPI_Comm mpiComm,
                            const int rankSource = 0);

} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOMPIFUNCTIONS_H_ */
