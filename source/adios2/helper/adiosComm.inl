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
#include <utility>   //std::pair

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

template <class T>
std::vector<T> Comm::AllGatherValues(const T source) const
{
    int size;
    SMPI_Comm_size(m_MPIComm, &size);
    std::vector<T> output(size);

    T sourceCopy = source; // so we can have an address for rvalues
    this->AllGatherArrays(&sourceCopy, 1, output.data());
    return output;
}

// AllGatherArrays full specializations implemented in 'adiosComm.tcc'.
template <>
void Comm::AllGatherArrays(const size_t *source, const size_t sourceCount,
                           size_t *destination) const;

// ReduceValues full specializations implemented in 'adiosComm.tcc'.
template <>
unsigned int Comm::ReduceValues(const unsigned int source, MPI_Op operation,
                                const int rankDestination) const;
template <>
unsigned long int Comm::ReduceValues(const unsigned long int source,
                                     MPI_Op operation,
                                     const int rankDestination) const;
template <>
unsigned long long int Comm::ReduceValues(const unsigned long long int source,
                                          MPI_Op operation,
                                          const int rankDestination) const;

// BroadcastValue full specializations implemented in 'adiosComm.tcc'.
template <>
size_t Comm::BroadcastValue(const size_t &input, const int rankSource) const;
template <>
std::string Comm::BroadcastValue(const std::string &input,
                                 const int rankSource) const;

// BroadcastVector full specializations implemented in 'adiosComm.tcc'.
template <>
void Comm::BroadcastVector(std::vector<char> &vector,
                           const int rankSource) const;
template <>
void Comm::BroadcastVector(std::vector<size_t> &vector,
                           const int rankSource) const;

template <typename TSend, typename TRecv>
void Comm::Allgather(const TSend *sendbuf, size_t sendcount, TRecv *recvbuf,
                     size_t recvcount, const std::string &hint) const
{
    return AllgatherImpl(sendbuf, sendcount, Datatype<TSend>(), recvbuf,
                         recvcount, Datatype<TRecv>(), hint);
}

template <typename T>
void Comm::Allreduce(const T *sendbuf, T *recvbuf, size_t count, MPI_Op op,
                     const std::string &hint) const
{
    return AllreduceImpl(sendbuf, recvbuf, count, Datatype<T>(), op, hint);
}

template <typename T>
void Comm::Bcast(T *buffer, const size_t count, int root,
                 const std::string &hint) const
{
    return BcastImpl(buffer, count, Datatype<T>(), root, hint);
}

template <typename T>
void Comm::Reduce(const T *sendbuf, T *recvbuf, size_t count, MPI_Op op,
                  int root, const std::string &hint) const
{
    return ReduceImpl(sendbuf, recvbuf, count, Datatype<T>(), op, root, hint);
}

template <typename T>
void Comm::ReduceInPlace(T *buf, size_t count, MPI_Op op, int root,
                         const std::string &hint) const
{
    return ReduceInPlaceImpl(buf, count, Datatype<T>(), op, root, hint);
}

template <typename T>
void Comm::Send(const T *buf, size_t count, int dest, int tag,
                const std::string &hint) const
{
    return SendImpl(buf, count, Datatype<T>(), dest, tag, hint);
}

template <typename T>
Comm::Status Comm::Recv(T *buf, size_t count, int source, int tag,
                        const std::string &hint) const
{
    return RecvImpl(buf, count, Datatype<T>(), source, tag, hint);
}

template <typename TSend, typename TRecv>
void Comm::Scatter(const TSend *sendbuf, size_t sendcount, TRecv *recvbuf,
                   size_t recvcount, int root, const std::string &hint) const
{
    return ScatterImpl(sendbuf, sendcount, Datatype<TSend>(), recvbuf,
                       recvcount, Datatype<TRecv>(), root, hint);
}

template <typename T>
Comm::Req Comm::Isend(const T *buffer, const size_t count, int dest, int tag,
                      const std::string &hint) const
{
    return IsendImpl(buffer, count, Datatype<T>(), dest, tag, hint);
}

template <typename T>
Comm::Req Comm::Irecv(T *buffer, const size_t count, int source, int tag,
                      const std::string &hint) const
{
    return IrecvImpl(buffer, count, Datatype<T>(), source, tag, hint);
}

// Datatype full specializations implemented in 'adiosComm.tcc'.
template <>
MPI_Datatype Comm::Datatype<signed char>();
template <>
MPI_Datatype Comm::Datatype<char>();
template <>
MPI_Datatype Comm::Datatype<short>();
template <>
MPI_Datatype Comm::Datatype<int>();
template <>
MPI_Datatype Comm::Datatype<long>();
template <>
MPI_Datatype Comm::Datatype<unsigned char>();
template <>
MPI_Datatype Comm::Datatype<unsigned short>();
template <>
MPI_Datatype Comm::Datatype<unsigned int>();
template <>
MPI_Datatype Comm::Datatype<unsigned long>();
template <>
MPI_Datatype Comm::Datatype<unsigned long long>();
template <>
MPI_Datatype Comm::Datatype<long long>();
template <>
MPI_Datatype Comm::Datatype<double>();
template <>
MPI_Datatype Comm::Datatype<long double>();
template <>
MPI_Datatype Comm::Datatype<std::pair<int, int>>();
template <>
MPI_Datatype Comm::Datatype<std::pair<float, int>>();
template <>
MPI_Datatype Comm::Datatype<std::pair<double, int>>();
template <>
MPI_Datatype Comm::Datatype<std::pair<long double, int>>();
template <>
MPI_Datatype Comm::Datatype<std::pair<short, int>>();

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSCOMM_INL_ */
