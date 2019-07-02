/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.tcc : specialization of template functions defined in
 * adiosMPIFunctions.h
 *
 *  Created on: Aug 30, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_
#define ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_

#include "adiosMPIFunctions.h"

#include <algorithm> //std::foreach

#include "adios2/common/ADIOSMPI.h"
#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace helper
{

template <>
std::vector<MPI_Request> Isend64<char>(const char *buffer, const size_t count,
                                       int dest, int tag, MPI_Comm mpiComm,
                                       const std::string &hint)
{

    std::vector<MPI_Request> requests(1);

    if (count > DefaultMaxFileBatchSize)
    {
        const size_t batches = count / DefaultMaxFileBatchSize;
        requests.resize(batches);

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            int batchSize = static_cast<int>(DefaultMaxFileBatchSize);
            CheckMPIReturn(MPI_Isend(const_cast<char *>(buffer + position),
                                     batchSize, MPI_CHAR, dest, tag, mpiComm,
                                     &requests[b]),
                           "in call to Isend64 batch " + std::to_string(b) +
                               " " + hint + "\n");

            position += DefaultMaxFileBatchSize;
        }
        const size_t remainder = count % DefaultMaxFileBatchSize;
        if (remainder > 0)
        {
            requests.resize(batches + 1);
            int batchSize = static_cast<int>(remainder);
            CheckMPIReturn(MPI_Isend(const_cast<char *>(buffer + position),
                                     batchSize, MPI_CHAR, dest, tag, mpiComm,
                                     &requests[batches]),
                           "in call to Isend64 remainder batch " + hint + "\n");
        }
    }
    else
    {
        int batchSize = static_cast<int>(count);
        CheckMPIReturn(MPI_Isend(const_cast<char *>(buffer), batchSize,
                                 MPI_CHAR, dest, tag, mpiComm, &requests[0]),
                       " in call to Isend64 with single batch " + hint + "\n");
    }

    return requests;
}

template <>
std::vector<MPI_Request> Irecv64<char>(char *buffer, const size_t count,
                                       int source, int tag, MPI_Comm mpiComm,
                                       const std::string &hint)
{
    std::vector<MPI_Request> requests(1);

    if (count > DefaultMaxFileBatchSize)
    {
        const size_t batches = count / DefaultMaxFileBatchSize;
        requests.resize(batches);
        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            int batchSize = static_cast<int>(DefaultMaxFileBatchSize);
            CheckMPIReturn(MPI_Irecv(buffer + position, batchSize, MPI_CHAR,
                                     source, tag, mpiComm, &requests[b]),
                           "in call to Irecv64 batch " + std::to_string(b) +
                               " " + hint + "\n");

            position += DefaultMaxFileBatchSize;
        }

        const size_t remainder = count % DefaultMaxFileBatchSize;
        if (remainder > 0)
        {
            requests.resize(batches + 1);
            int batchSize = static_cast<int>(remainder);
            CheckMPIReturn(MPI_Irecv(buffer + position, batchSize, MPI_CHAR,
                                     source, tag, mpiComm, &requests[batches]),
                           "in call to Irecv64 remainder batch " + hint + "\n");
        }
    }
    else
    {
        int batchSize = static_cast<int>(count);
        CheckMPIReturn(MPI_Irecv(buffer, batchSize, MPI_CHAR, source, tag,
                                 mpiComm, &requests[0]),
                       " in call to Isend64 with single batch " + hint + "\n");
    }
    return requests;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMPIFUNCTIONS_TCC_ */
