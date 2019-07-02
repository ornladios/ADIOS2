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

// BroadcastValue specializations
template <>
size_t BroadcastValue(const size_t &input, MPI_Comm mpiComm,
                      const int rankSource)
{
    int rank;
    SMPI_Comm_rank(mpiComm, &rank);
    size_t output = 0;

    if (rank == rankSource)
    {
        output = input;
    }

    SMPI_Bcast(&output, 1, ADIOS2_MPI_SIZE_T, rankSource, mpiComm);

    return output;
}

template <>
std::string BroadcastValue(const std::string &input, MPI_Comm mpiComm,
                           const int rankSource)
{
    int rank;
    SMPI_Comm_rank(mpiComm, &rank);
    const size_t inputSize = input.size();
    const size_t length = BroadcastValue(inputSize, mpiComm, rankSource);
    std::string output;

    if (rank == rankSource)
    {
        output = input;
    }
    else
    {
        output.resize(length);
    }

    SMPI_Bcast(const_cast<char *>(output.data()), static_cast<int>(length),
               MPI_CHAR, rankSource, mpiComm);

    return output;
}

// ReduceValue specializations
template <>
unsigned int ReduceValues(const unsigned int source, MPI_Comm mpiComm,
                          MPI_Op operation, const int rankDestination)
{
    unsigned int sourceLocal = source;
    unsigned int reduceValue = 0;
    SMPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED, operation,
                rankDestination, mpiComm);
    return reduceValue;
}

template <>
unsigned long int ReduceValues(const unsigned long int source, MPI_Comm mpiComm,
                               MPI_Op operation, const int rankDestination)
{
    unsigned long int sourceLocal = source;
    unsigned long int reduceValue = 0;
    SMPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED_LONG, operation,
                rankDestination, mpiComm);
    return reduceValue;
}

template <>
unsigned long long int ReduceValues(const unsigned long long int source,
                                    MPI_Comm mpiComm, MPI_Op operation,
                                    const int rankDestination)
{
    unsigned long long int sourceLocal = source;
    unsigned long long int reduceValue = 0;
    SMPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED_LONG_LONG,
                operation, rankDestination, mpiComm);
    return reduceValue;
}

// BroadcastVector specializations
template <>
void BroadcastVector(std::vector<char> &vector, MPI_Comm mpiComm,
                     const int rankSource)
{
    int size;
    SMPI_Comm_size(mpiComm, &size);

    if (size == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = BroadcastValue(vector.size(), mpiComm, rankSource);
    int rank;
    SMPI_Comm_rank(mpiComm, &rank);

    if (rank != rankSource)
    {
        vector.resize(inputSize);
    }

    const int MAXBCASTSIZE = 1073741824;
    size_t blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    char *buffer = vector.data();
    while (inputSize > 0)
    {
        SMPI_Bcast(buffer, static_cast<int>(blockSize), MPI_CHAR, rankSource,
                   mpiComm);
        buffer += blockSize;
        inputSize -= blockSize;
        blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    }
}

// BroadcastVector specializations
template <>
void BroadcastVector(std::vector<size_t> &vector, MPI_Comm mpiComm,
                     const int rankSource)
{
    int size;
    SMPI_Comm_size(mpiComm, &size);

    if (size == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = BroadcastValue(vector.size(), mpiComm, rankSource);
    int rank;
    SMPI_Comm_rank(mpiComm, &rank);

    if (rank != rankSource)
    {
        vector.resize(inputSize);
    }

    const int MAXBCASTSIZE = 1073741824 / sizeof(size_t);
    size_t blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    size_t *buffer = vector.data();
    while (inputSize > 0)
    {
        SMPI_Bcast(buffer, static_cast<int>(blockSize), ADIOS2_MPI_SIZE_T,
                   rankSource, mpiComm);
        buffer += blockSize;
        inputSize -= blockSize;
        blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    }
}

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
