/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIAggregator.cpp
 *
 *  Created on: Feb 20, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "MPIAggregator.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace aggregator
{

MPIAggregator::MPIAggregator() : m_Comm(MPI_COMM_NULL) {}

MPIAggregator::~MPIAggregator()
{
    if (m_IsActive)
    {
        helper::CheckMPIReturn(MPI_Comm_free(&m_Comm),
                               "freeing aggregators comm in MPIAggregator "
                               "destructor, not recommended");
    }
}

void MPIAggregator::Init(const size_t subStreams, MPI_Comm parentComm) {}

void MPIAggregator::SwapBuffers(const int step) noexcept {}

void MPIAggregator::ResetBuffers() noexcept {}

format::Buffer &MPIAggregator::GetConsumerBuffer(format::Buffer &buffer)
{
    return buffer;
}

void MPIAggregator::Close()
{
    if (m_IsActive)
    {
        helper::CheckMPIReturn(MPI_Comm_free(&m_Comm),
                               "freeing aggregators comm at Close\n");
        m_IsActive = false;
    }
}

// PROTECTED
void MPIAggregator::InitComm(const size_t subStreams, MPI_Comm parentComm)
{
    int parentRank;
    int parentSize;
    MPI_Comm_rank(parentComm, &parentRank);
    MPI_Comm_size(parentComm, &parentSize);

    const size_t process = static_cast<size_t>(parentRank);
    const size_t processes = static_cast<size_t>(parentSize);

    // Divide the processes into S=subStreams groups.
    const size_t q = processes / subStreams;
    const size_t r = processes % subStreams;

    // Groups [0,r) have size q+1.  Groups [r,S) have size q.
    const size_t first_in_small_groups = r * (q + 1);

    // Within each group the first process becomes its consumer.
    if (process >= first_in_small_groups)
    {
        m_SubStreamIndex = r + (process - first_in_small_groups) / q;
        m_ConsumerRank = static_cast<int>(first_in_small_groups +
                                          (m_SubStreamIndex - r) * q);
    }
    else
    {
        m_SubStreamIndex = process / (q + 1);
        m_ConsumerRank = static_cast<int>(m_SubStreamIndex * (q + 1));
    }

    helper::CheckMPIReturn(
        MPI_Comm_split(parentComm, m_ConsumerRank, parentRank, &m_Comm),
        "creating aggregators comm with split at Open");

    MPI_Comm_rank(m_Comm, &m_Rank);
    MPI_Comm_size(m_Comm, &m_Size);

    if (m_Rank != 0)
    {
        m_IsConsumer = false;
    }

    m_IsActive = true;
    m_SubStreams = subStreams;
}

void MPIAggregator::HandshakeRank(const int rank)
{
    int message = -1;
    if (m_Rank == rank)
    {
        message = m_Rank;
    }

    helper::CheckMPIReturn(MPI_Bcast(&message, 1, MPI_INT, rank, m_Comm),
                           "handshake with aggregator rank 0 at Open");
}

} // end namespace aggregator
} // end namespace adios2
