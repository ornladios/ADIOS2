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

#include "adios2/ADIOSMPI.h"
#include "adios2/helper/adiosFunctions.h" //helper::CheckMPIReturn

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

void MPIChain::IExchange(BufferSTL &bufferSTL, const int step)
{
    if (m_Size == 1)
    {
        return;
    }

    BufferSTL &sendBuffer = GetSender(bufferSTL);
    const int endRank = m_Size - 1 - step;
    const bool sender = (m_Rank >= 1 && m_Rank <= endRank) ? true : false;
    const bool receiver = (m_Rank < endRank) ? true : false;

    if (sender) // sender
    {
        helper::CheckMPIReturn(MPI_Isend(&sendBuffer.m_Position, 1,
                                         ADIOS2_MPI_SIZE_T, m_Rank - 1, 0,
                                         m_Comm, &m_DataRequests[0]),
                               ", aggregation Isend size at iteration " +
                                   std::to_string(step) + "\n");

        helper::CheckMPIReturn(
            MPI_Isend(sendBuffer.m_Buffer.data(),
                      static_cast<int>(sendBuffer.m_Position), MPI_CHAR,
                      m_Rank - 1, 1, m_Comm, &m_DataRequests[1]),
            ", aggregation Isend data at iteration " + std::to_string(step) +
                "\n");
    }
    // receive size, resize receiving buffer and receive data
    if (receiver)
    {
        size_t bufferSize = 0;
        MPI_Request receiveSizeRequest;
        helper::CheckMPIReturn(MPI_Irecv(&bufferSize, 1, ADIOS2_MPI_SIZE_T,
                                         m_Rank + 1, 0, m_Comm,
                                         &receiveSizeRequest),
                               ", aggregation Irecv size at iteration " +
                                   std::to_string(step) + "\n");

        helper::CheckMPIReturn(
            MPI_Wait(&receiveSizeRequest, MPI_STATUS_IGNORE),
            ", aggregation waiting for receiver size at iteration " +
                std::to_string(step) + "\n");

        BufferSTL &receiveBuffer = GetReceiver(bufferSTL);
        ResizeUpdateBufferSTL(
            bufferSize, receiveBuffer,
            "in aggregation, when resizing receiving buffer to size " +
                std::to_string(bufferSize));

        helper::CheckMPIReturn(
            MPI_Irecv(receiveBuffer.m_Buffer.data(),
                      static_cast<int>(receiveBuffer.m_Position), MPI_CHAR,
                      m_Rank + 1, 1, m_Comm, &m_DataRequests[2]),
            ", aggregation Irecv data at iteration " + std::to_string(step) +
                "\n");
    }
}

void MPIChain::Wait(const int step)
{
    if (m_Size == 1)
    {
        return;
    }

    const int endRank = m_Size - 1 - step;
    const bool sender = (m_Rank >= 1 && m_Rank <= endRank) ? true : false;
    const bool receiver = (m_Rank < endRank) ? true : false;

    if (receiver)
    {
        helper::CheckMPIReturn(
            MPI_Wait(&m_DataRequests[2], MPI_STATUS_IGNORE),
            ", aggregation waiting for receiver data at iteration " +
                std::to_string(step) + "\n");
    }

    if (sender)
    {
        helper::CheckMPIReturn(
            MPI_Wait(&m_DataRequests[0], MPI_STATUS_IGNORE),
            ", aggregation waiting for sender size at iteration " +
                std::to_string(step) + "\n");

        helper::CheckMPIReturn(
            MPI_Wait(&m_DataRequests[1], MPI_STATUS_IGNORE),
            ", aggregation waiting for sender data at iteration " +
                std::to_string(step) + "\n");
    }
}

void MPIChain::SwapBuffers(const int /*step*/) noexcept
{
    m_CurrentBufferOrder = (m_CurrentBufferOrder == 0) ? 1 : 0;
}

void MPIChain::ResetBuffers() noexcept { m_CurrentBufferOrder = 0; }

BufferSTL &MPIChain::GetConsumerBuffer(BufferSTL &bufferSTL)
{
    return GetSender(bufferSTL);
}

// PRIVATE
void MPIChain::HandshakeLinks()
{
    int link = -1;

    MPI_Request sendRequest;
    if (m_Rank > 0) // send
    {
        helper::CheckMPIReturn(
            MPI_Isend(&m_Rank, 1, MPI_INT, m_Rank - 1, 0, m_Comm, &sendRequest),
            "Isend handshake with neighbor, MPIChain aggregator, at Open");
    }

    if (m_Rank < m_Size - 1) // receive
    {
        MPI_Request receiveRequest;
        helper::CheckMPIReturn(
            MPI_Irecv(&link, 1, MPI_INT, m_Rank + 1, 0, m_Comm,
                      &receiveRequest),
            "Irecv handshake with neighbor, MPIChain aggregator, at Open");

        helper::CheckMPIReturn(
            MPI_Wait(&receiveRequest, MPI_STATUS_IGNORE),
            "Irecv Wait handshake with neighbor, MPIChain aggregator, at Open");
    }

    if (m_Rank > 0)
    {
        helper::CheckMPIReturn(
            MPI_Wait(&sendRequest, MPI_STATUS_IGNORE),
            "Isend wait handshake with neighbor, MPIChain aggregator, at Open");
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
