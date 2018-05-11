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
#include "adios2/helper/adiosFunctions.h" //CheckMPIReturn

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

std::vector<MPI_Request> MPIChain::IExchange(BufferSTL &bufferSTL,
                                             const int step)
{
    BufferSTL &sendBuffer = GetSender(bufferSTL);
    const int endRank = m_Size - 1 - step;
    const bool sender = (m_Rank >= 1 && m_Rank <= endRank) ? true : false;
    const bool receiver = (m_Rank < endRank) ? true : false;

    std::vector<MPI_Request> requests(3);

    if (sender) // sender
    {
        CheckMPIReturn(MPI_Isend(&sendBuffer.m_Position, 1, ADIOS2_MPI_SIZE_T,
                                 m_Rank - 1, 0, m_Comm, &requests[0]),
                       ", aggregation Isend size at iteration " +
                           std::to_string(step) + "\n");

        CheckMPIReturn(MPI_Isend(sendBuffer.m_Buffer.data(),
                                 static_cast<int>(sendBuffer.m_Position),
                                 MPI_CHAR, m_Rank - 1, 1, m_Comm, &requests[1]),
                       ", aggregation Isend data at iteration " +
                           std::to_string(step) + "\n");
    }
    // receive size, resize receiving buffer and receive data
    if (receiver)
    {
        size_t bufferSize = 0;
        MPI_Request receiveSizeRequest;
        CheckMPIReturn(MPI_Irecv(&bufferSize, 1, ADIOS2_MPI_SIZE_T, m_Rank + 1,
                                 0, m_Comm, &receiveSizeRequest),
                       ", aggregation Irecv size at iteration " +
                           std::to_string(step) + "\n");

        MPI_Status receiveStatus;
        CheckMPIReturn(MPI_Wait(&receiveSizeRequest, &receiveStatus),
                       ", aggregation waiting for receiver size at iteration " +
                           std::to_string(step) + "\n");

        BufferSTL &receiveBuffer = GetReceiver(bufferSTL);
        ResizeUpdateBufferSTL(
            bufferSize, receiveBuffer,
            "in aggregation, when resizing receiving buffer to size " +
                std::to_string(bufferSize));

        CheckMPIReturn(MPI_Irecv(receiveBuffer.m_Buffer.data(),
                                 static_cast<int>(receiveBuffer.m_Position),
                                 MPI_CHAR, m_Rank + 1, 1, m_Comm, &requests[2]),
                       ", aggregation Irecv data at iteration " +
                           std::to_string(step) + "\n");
    }

    return requests;
}

void MPIChain::Wait(std::vector<MPI_Request> &requests, const int step)
{
    const int endRank = m_Size - 1 - step;
    const bool sender = (m_Rank >= 1 && m_Rank <= endRank) ? true : false;
    const bool receiver = (m_Rank < endRank) ? true : false;

    MPI_Status status;
    if (receiver)
    {
        CheckMPIReturn(MPI_Wait(&requests[2], &status),
                       ", aggregation waiting for receiver data at iteration " +
                           std::to_string(step) + "\n");
    }

    if (sender)
    {
        CheckMPIReturn(MPI_Wait(&requests[0], &status),
                       ", aggregation waiting for sender size at iteration " +
                           std::to_string(step) + "\n");

        CheckMPIReturn(MPI_Wait(&requests[1], &status),
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
        MPI_Isend(&m_Rank, 1, MPI_INT, m_Rank - 1, 0, m_Comm, &sendRequest);
    }

    if (m_Rank < m_Size - 1) // receive
    {
        MPI_Request receiveRequest;
        MPI_Irecv(&link, 1, MPI_INT, m_Rank + 1, 0, m_Comm, &receiveRequest);

        MPI_Status receiveStatus;
        MPI_Wait(&receiveRequest, &receiveStatus);
    }

    if (m_Rank > 0)
    {
        MPI_Status sendStatus;
        MPI_Wait(&sendRequest, &sendStatus);
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
