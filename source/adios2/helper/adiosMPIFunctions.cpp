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

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

std::string BroadcastString(const std::string &input, MPI_Comm mpiComm,
                            const int rankSource)
{
    int rank;
    MPI_Comm_rank(mpiComm, &rank);
    size_t length = 0;
    std::string output;

    if (rank == rankSource)
    {
        length = input.size();
        MPI_Bcast(&length, 1, ADIOS2_MPI_SIZE_T, rankSource, mpiComm);

        MPI_Bcast(const_cast<char *>(input.data()), length, MPI_CHAR,
                  rankSource, mpiComm);

        return input;
    }
    else
    {
        MPI_Bcast(&length, 1, ADIOS2_MPI_SIZE_T, rankSource, mpiComm);
        output.resize(length);
        MPI_Bcast(const_cast<char *>(output.data()), length, MPI_CHAR,
                  rankSource, mpiComm);
    }
    return output;
}

} // end namespace adios2
