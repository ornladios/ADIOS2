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

#include "adios2/ADIOSMPI.h"

namespace adios2
{
namespace aggregator
{

MPIAggregator::MPIAggregator() : m_Comm(MPI_COMM_SELF) {}

MPIAggregator::~MPIAggregator()
{
    if (m_IsActive)
    {
        MPI_Comm_free(&m_Comm);
    }
}

void MPIAggregator::Init(const size_t subStreams, MPI_Comm parentComm) {}

void MPIAggregator::Send(BufferSTL &bufferSTL, const int step) {}

void MPIAggregator::Receive(BufferSTL &bufferSTL, const int step) {}

void MPIAggregator::SwapBufferOrder() {}

BufferSTL &MPIAggregator::GetConsumerBuffer(BufferSTL &bufferSTL)
{
    return bufferSTL;
}

void MPIAggregator::Close()
{
    if (m_IsActive)
    {
        MPI_Comm_free(&m_Comm);
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

    const size_t processes = static_cast<size_t>(parentSize);
    const size_t stride = processes / subStreams;
    const size_t remainder = processes % subStreams;

    for (auto s = 0; s < subStreams; ++s)
    {
        const size_t consumer = s * stride; // aggregator rank is start
        size_t producers = stride;
        if (s == subStreams - 1)
        {
            producers += remainder;
        }

        if (static_cast<size_t>(parentRank) >= consumer &&
            static_cast<size_t>(parentRank) < consumer + producers)
        {
            MPI_Comm_split(parentComm, static_cast<int>(consumer), parentRank,
                           &m_Comm);
            m_ConsumerRank = static_cast<int>(consumer);
        }
    }

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

    MPI_Bcast(&message, 1, MPI_INT, rank, m_Comm);
}

} // end namespace aggregator
} // end namespace adios2
