/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosComm.inl
 */

#ifndef ADIOS2_HELPER_ADIOSCOMM_INL_
#define ADIOS2_HELPER_ADIOSCOMM_INL_
#ifndef ADIOS2_HELPER_ADIOSCOMM_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include <numeric>   //std::accumulate
#include <stdexcept> //std::runtime_error

namespace adios2
{
namespace helper
{

// GatherArrays full specializations implemented in 'adiosComm.tcc'.
template <>
void Comm::GatherArrays(const char *source, size_t sourceCount,
                        char *destination, int rankDestination) const;
template <>
void Comm::GatherArrays(const size_t *source, size_t sourceCount,
                        size_t *destination, int rankDestination) const;

template <class T>
std::vector<T> Comm::GatherValues(T source, int rankDestination) const
{
    int rank, size;
    SMPI_Comm_rank(m_MPIComm, &rank);
    SMPI_Comm_size(m_MPIComm, &size);

    std::vector<T> output;

    if (rank == rankDestination) // pre-allocate in destination rank
    {
        output.resize(size);
    }

    T sourceCopy = source; // so we can have an address for rvalues
    this->GatherArrays(&sourceCopy, 1, output.data(), rankDestination);

    return output;
}

// GathervArrays full specializations implemented in 'adiosComm.tcc'.
template <>
void Comm::GathervArrays(const char *source, size_t sourceCount,
                         const size_t *counts, size_t countsSize,
                         char *destination, int rankDestination) const;
template <>
void Comm::GathervArrays(const size_t *source, size_t sourceCount,
                         const size_t *counts, size_t countsSize,
                         size_t *destination, int rankDestination) const;

template <class T>
void Comm::GathervVectors(const std::vector<T> &in, std::vector<T> &out,
                          size_t &position, int rankDestination,
                          size_t extraSize) const
{
    const size_t inSize = in.size();
    const std::vector<size_t> counts =
        this->GatherValues(inSize, rankDestination);

    size_t gatheredSize = 0;

    int rank;
    SMPI_Comm_rank(m_MPIComm, &rank);

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

    this->GathervArrays(in.data(), in.size(), counts.data(), counts.size(),
                        out.data() + position);
    position += gatheredSize;
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSCOMM_INL_ */
