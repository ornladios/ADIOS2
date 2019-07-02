/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.tcc : specialization of template functions defined in
 * adiosComm.h
 */

#ifndef ADIOS2_HELPER_ADIOSCOMM_TCC_
#define ADIOS2_HELPER_ADIOSCOMM_TCC_

#include "adiosComm.h"

#include "adios2/common/ADIOSMPI.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/helper/adiosType.h"

#include <stdexcept> //std::runtime_error

namespace adios2
{
namespace helper
{

namespace
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

}

// GatherArrays full specializations forward-declared in 'adiosComm.inl'.
template <>
void Comm::GatherArrays(const char *source, size_t sourceCount,
                        char *destination, int rankDestination) const
{
    int countsInt = static_cast<int>(sourceCount);
    int result = SMPI_Gather(const_cast<char *>(source), countsInt, MPI_CHAR,
                             destination, countsInt, MPI_CHAR, rankDestination,
                             m_MPIComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type MPI_CHAR function\n");
    }
}

template <>
void Comm::GatherArrays(const size_t *source, size_t sourceCount,
                        size_t *destination, int rankDestination) const
{
    int countsInt = static_cast<int>(sourceCount);
    int result = SMPI_Gather(const_cast<size_t *>(source), countsInt,
                             ADIOS2_MPI_SIZE_T, destination, countsInt,
                             ADIOS2_MPI_SIZE_T, rankDestination, m_MPIComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type size_t function\n");
    }
}

// GathervArrays full specializations forward-declared in 'adiosComm.inl'.
template <>
void Comm::GathervArrays(const char *source, size_t sourceCount,
                         const size_t *counts, size_t countsSize,
                         char *destination, int rankDestination) const
{
    int result = 0;
    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);

    std::vector<int> countsInt, displacementsInt;

    if (rank == rankDestination)
    {
        countsInt = NewVectorTypeFromArray<size_t, int>(counts, countsSize);
        displacementsInt = GetGathervDisplacements(counts, countsSize);
    }

    int sourceCountInt = static_cast<int>(sourceCount);
    result =
        SMPI_Gatherv(const_cast<char *>(source), sourceCountInt, MPI_CHAR,
                     destination, countsInt.data(), displacementsInt.data(),
                     MPI_CHAR, rankDestination, m_MPIComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gatherv type MPI_CHAR function\n");
    }
}

template <>
void Comm::GathervArrays(const size_t *source, size_t sourceCount,
                         const size_t *counts, size_t countsSize,
                         size_t *destination, int rankDestination) const
{
    int result = 0;
    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);

    std::vector<int> countsInt =
        NewVectorTypeFromArray<size_t, int>(counts, countsSize);

    std::vector<int> displacementsInt =
        GetGathervDisplacements(counts, countsSize);

    int sourceCountInt = static_cast<int>(sourceCount);

    result = SMPI_Gatherv(const_cast<size_t *>(source), sourceCountInt,
                          ADIOS2_MPI_SIZE_T, destination, countsInt.data(),
                          displacementsInt.data(), ADIOS2_MPI_SIZE_T,
                          rankDestination, m_MPIComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Gather type size_t function\n");
    }
}

// AllGatherArrays full specializations forward-declared in 'adiosComm.inl'.
template <>
void Comm::AllGatherArrays(const size_t *source, const size_t sourceCount,
                           size_t *destination) const
{
    int countsInt = static_cast<int>(sourceCount);
    int result = MPI_Allgather(const_cast<size_t *>(source), countsInt,
                               ADIOS2_MPI_SIZE_T, destination, countsInt,
                               ADIOS2_MPI_SIZE_T, m_MPIComm);

    if (result != MPI_SUCCESS)
    {
        throw std::runtime_error("ERROR: in ADIOS2 detected failure in MPI "
                                 "Allgather type size_t function\n");
    }
}

// ReduceValues full specializations forward-declared in 'adiosComm.inl'.
template <>
unsigned int Comm::ReduceValues(const unsigned int source, MPI_Op operation,
                                const int rankDestination) const
{
    unsigned int sourceLocal = source;
    unsigned int reduceValue = 0;
    SMPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED, operation,
                rankDestination, m_MPIComm);
    return reduceValue;
}

template <>
unsigned long int Comm::ReduceValues(const unsigned long int source,
                                     MPI_Op operation,
                                     const int rankDestination) const
{
    unsigned long int sourceLocal = source;
    unsigned long int reduceValue = 0;
    SMPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED_LONG, operation,
                rankDestination, m_MPIComm);
    return reduceValue;
}

template <>
unsigned long long int Comm::ReduceValues(const unsigned long long int source,
                                          MPI_Op operation,
                                          const int rankDestination) const
{
    unsigned long long int sourceLocal = source;
    unsigned long long int reduceValue = 0;
    SMPI_Reduce(&sourceLocal, &reduceValue, 1, MPI_UNSIGNED_LONG_LONG,
                operation, rankDestination, m_MPIComm);
    return reduceValue;
}

// BroadcastValue full specializations forward-declared in 'adiosComm.inl'.
template <>
size_t Comm::BroadcastValue(const size_t &input, const int rankSource) const
{
    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);
    size_t output = 0;

    if (rank == rankSource)
    {
        output = input;
    }

    SMPI_Bcast(&output, 1, ADIOS2_MPI_SIZE_T, rankSource, m_MPIComm);

    return output;
}

template <>
std::string Comm::BroadcastValue(const std::string &input,
                                 const int rankSource) const
{
    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);
    const size_t inputSize = input.size();
    const size_t length = this->BroadcastValue(inputSize, rankSource);
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
               MPI_CHAR, rankSource, m_MPIComm);

    return output;
}

// BroadcastVector full specializations forward-declared in 'adiosComm.inl'.
template <>
void Comm::BroadcastVector(std::vector<char> &vector,
                           const int rankSource) const
{
    int size;
    SMPI_Comm_size(m_MPIComm, &size);

    if (size == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = this->BroadcastValue(vector.size(), rankSource);
    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);

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
                   m_MPIComm);
        buffer += blockSize;
        inputSize -= blockSize;
        blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    }
}

template <>
void Comm::BroadcastVector(std::vector<size_t> &vector,
                           const int rankSource) const
{
    int size;
    SMPI_Comm_size(m_MPIComm, &size);

    if (size == 1)
    {
        return;
    }

    // First Broadcast the size, then the contents
    size_t inputSize = this->BroadcastValue(vector.size(), rankSource);
    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);

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
                   rankSource, m_MPIComm);
        buffer += blockSize;
        inputSize -= blockSize;
        blockSize = (inputSize > MAXBCASTSIZE ? MAXBCASTSIZE : inputSize);
    }
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSCOMM_TCC_ */
