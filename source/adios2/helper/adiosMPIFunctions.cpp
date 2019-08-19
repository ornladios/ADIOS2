/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.cpp
 *
 *  Created on: Jul 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "adiosMPIFunctions.h"
#include "adiosMPIFunctions.tcc"

#include "adios2/common/ADIOSMPI.h"
#include "adios2/common/ADIOSTypes.h"

#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace helper
{

std::vector<int> GetGathervDisplacements(const size_t *counts,
                                         const size_t countsSize)
{
    std::vector<int> displacements(countsSize);
    displacements[0] = 0;

    for (size_t i = 1; i < countsSize; ++i)
    {
        displacements[i] =
            displacements[i - 1] + static_cast<int>(counts[i - 1]);
    }
    return displacements;
}

void CheckMPIReturn(const int value, const std::string hint)
{
    if (value == MPI_SUCCESS)
    {
        return;
    }

    std::string error;
    switch (value)
    {
    case MPI_ERR_COMM:
        error = "MPI_ERR_COMM";
        break;
    case MPI_ERR_INTERN:
        error = "MPI_ERR_INTERN";
        break;
    default:
        error = "MPI_ERR number: " + std::to_string(value);
    }

    throw std::runtime_error("ERROR: ADIOS2 detected " + error + ", " + hint);
}

std::string BroadcastFile(const std::string &fileName, MPI_Comm mpiComm,
                          const std::string hint, const int rankSource)
{
    int rank;
    MPI_Comm_rank(mpiComm, &rank);
    std::string fileContents;

    // Read the file on rank 0 and broadcast it to everybody else
    if (rank == rankSource)
    {
        // load file contents
        fileContents = FileToString(fileName, hint);
    }
    fileContents = BroadcastValue(fileContents, mpiComm, rankSource);

    return fileContents;
}

} // end namespace helper
} // end namespace adios2
