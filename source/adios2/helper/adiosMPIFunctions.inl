/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.inl
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_INL_
#define ADIOS2_HELPER_ADIOSMPIFUNCTIONS_INL_
#ifndef ADIOS2_HELPER_ADIOSMPIFUNCTIONS_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include <numeric>   //std::accumulate
#include <stdexcept> //std::runtime_error

namespace adios2
{
namespace helper
{

template <class T>
std::vector<T> GatherValues(const T source, MPI_Comm mpiComm,
                            const int rankDestination)
{
    int rank, size;
    SMPI_Comm_rank(mpiComm, &rank);
    SMPI_Comm_size(mpiComm, &size);

    std::vector<T> output;

    if (rank == rankDestination) // pre-allocate in destination rank
    {
        output.resize(size);
    }

    T sourceCopy = source; // so we can have an address for rvalues
    GatherArrays(&sourceCopy, 1, output.data(), mpiComm, rankDestination);

    return output;
}

template <class T>
std::vector<T> AllGatherValues(const T source, MPI_Comm mpiComm)
{
    int size;
    SMPI_Comm_size(mpiComm, &size);
    std::vector<T> output(size);

    T sourceCopy = source; // so we can have an address for rvalues
    AllGatherArrays(&sourceCopy, 1, output.data(), mpiComm);
    return output;
}

template <class T>
void GathervVectors(const std::vector<T> &in, std::vector<T> &out,
                    size_t &position, MPI_Comm mpiComm,
                    const int rankDestination, const size_t extraSize)
{
    const size_t inSize = in.size();
    const std::vector<size_t> counts =
        GatherValues(inSize, mpiComm, rankDestination);

    size_t gatheredSize = 0;

    int rank;
    SMPI_Comm_rank(mpiComm, &rank);

    if (rank == rankDestination) // pre-allocate vector
    {
        gatheredSize = std::accumulate(counts.begin(), counts.end(), size_t(0));

        const size_t newSize = position + gatheredSize;
        try
        {
            out.reserve(newSize + extraSize); // to avoid power of 2 growth
            out.resize(newSize + extraSize);
        }
        catch (...)
        {
            std::throw_with_nested(
                std::runtime_error("ERROR: buffer overflow when resizing to " +
                                   std::to_string(newSize) +
                                   " bytes, in call to GathervVectors\n"));
        }
    }

    GathervArrays(in.data(), in.size(), counts.data(), counts.size(),
                  out.data() + position, mpiComm);
    position += gatheredSize;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMPIFUNCTIONS_INL_ */
