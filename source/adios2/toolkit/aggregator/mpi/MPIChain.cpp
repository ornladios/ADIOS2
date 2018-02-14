/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIChain.cpp
 *
 *  Created on: Feb 21, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "MPIChain.h"

#include <iostream>

#include "adios2/ADIOSMPI.h"

namespace adios2
{
namespace aggregator
{

MPIChain::MPIChain() : MPIAggregator() {}

void MPIChain::Init(const size_t subStreams, MPI_Comm parentComm)
{
    InitComm(subStreams, parentComm);
    HandshakeRank(0);
    HandshakeLinks();

    // add a receiving buffer except for the last rank (only sends)
    if (m_Rank < m_Size)
    {
        m_Buffers.emplace_back(); // just one for now
    }
}

void MPIChain::Send(BufferSTL &bufferSTL, const int step)
{
    BufferSTL &sendBuffer = GetSender(bufferSTL);

    const int finalRank = m_Size - step;

    // send size
    if (m_Rank >= 1 && m_Rank < finalRank) // sender
    {
        MPI_Request sendRequest;
        MPI_Isend(&sendBuffer.m_Position, 1, ADIOS2_MPI_SIZE_T, m_Rank - 1, 0,
                  m_Comm, &sendRequest);
    }
    // receive size and resize receiving buffer
    if (m_Rank >= 0 && m_Rank < finalRank - 1) // receiver
    {
        size_t bufferSize = 0;
        MPI_Request receiveRequest;
        MPI_Irecv(&bufferSize, 1, ADIOS2_MPI_SIZE_T, m_Rank + 1, 0, m_Comm,
                  &receiveRequest);

        MPI_Status receiveStatus;
        MPI_Wait(&receiveRequest, &receiveStatus);

        BufferSTL &receiveBuffer = GetReceiver(bufferSTL);
        ResizeUpdateBufferSTL(
            bufferSize, receiveBuffer,
            "in aggregation, when resizing receiving buffer to size " +
                std::to_string(bufferSize));
    }

    // send data
    if (m_Rank >= 1 && m_Rank < finalRank) // sender
    {
        MPI_Request sendRequest;
        MPI_Isend(sendBuffer.m_Buffer.data(),
                  static_cast<int>(sendBuffer.m_Position), MPI_CHAR, m_Rank - 1,
                  1, m_Comm, &sendRequest);
    }
}

void MPIChain::Receive(BufferSTL &bufferSTL, const int step)
{
    const int finalRank = m_Size - step;
    if (m_Rank >= 0 && m_Rank < finalRank - 1) // receiver
    {
        BufferSTL &receiveBuffer = GetReceiver(bufferSTL);

        MPI_Request receiveRequest;
        MPI_Irecv(receiveBuffer.m_Buffer.data(),
                  static_cast<int>(receiveBuffer.m_Position), MPI_CHAR,
                  m_Rank + 1, 1, m_Comm, &receiveRequest);

        MPI_Status receiveStatus;
        MPI_Wait(&receiveRequest, &receiveStatus);
    }
}

void MPIChain::SwapBufferOrder()
{
    m_CurrentBufferOrder = (m_CurrentBufferOrder == 0) ? 1 : 0;
}

BufferSTL &MPIChain::GetConsumerBuffer(BufferSTL &bufferSTL)
{
    return GetSender(bufferSTL);
}

// PRIVATE
void MPIChain::HandshakeLinks()
{
    int link = -1;

    if (m_Rank > 0) // send
    {
        MPI_Request sendRequest;
        MPI_Isend(&m_Rank, 1, MPI_INT, m_Rank - 1, 0, m_Comm, &sendRequest);
    }

    if (m_Rank < m_Size - 1) // receive
    {
        MPI_Request receiveRequest;
        MPI_Irecv(&link, 1, MPI_INT, m_Rank + 1, 0, m_Comm, &receiveRequest);

        MPI_Status receiveStatus;
        MPI_Wait(&receiveRequest, &receiveStatus);
    }
}

BufferSTL &MPIChain::GetSender(BufferSTL &bufferSTL)
{
    if (m_CurrentBufferOrder == 0)
    {
        return bufferSTL;
    }
    else
    {
        return m_Buffers.front();
    }
}

BufferSTL &MPIChain::GetReceiver(BufferSTL &bufferSTL)
{
    if (m_CurrentBufferOrder == 0)
    {
        return m_Buffers.front();
    }
    else
    {
        return bufferSTL;
    }
}

void MPIChain::ResizeUpdateBufferSTL(const size_t newSize, BufferSTL &bufferSTL,
                                     const std::string hint)
{
    bufferSTL.Resize(newSize, hint);
    bufferSTL.m_Position = bufferSTL.m_Buffer.size();
}

} // end namespace aggregator
} // end namespace adios2
