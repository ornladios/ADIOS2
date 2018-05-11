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

std::vector<MPI_Request> MPIAggregator::IExchange(BufferSTL & /**bufferSTL*/,
                                                  const int /** step*/)
{
    std::vector<MPI_Request> requests;
    return requests;
}

void MPIAggregator::Wait(std::vector<MPI_Request> & /**request*/,
                         const int /**step*/)
{
}

void MPIAggregator::SwapBuffers(const int step) noexcept {}

void MPIAggregator::ResetBuffers() noexcept {}

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
    size_t stride = processes / subStreams + 1;
    const size_t remainder = processes % subStreams;

    size_t consumer = 0;

    for (auto s = 0; s < subStreams; ++s)
    {
        if (s >= remainder)
        {
            stride = processes / subStreams;
        }

        if (static_cast<size_t>(parentRank) >= consumer &&
            static_cast<size_t>(parentRank) < consumer + stride)
        {
            MPI_Comm_split(parentComm, static_cast<int>(consumer), parentRank,
                           &m_Comm);
            m_ConsumerRank = static_cast<int>(consumer);
            m_SubStreamIndex = static_cast<size_t>(s);
        }

        consumer += stride;
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
