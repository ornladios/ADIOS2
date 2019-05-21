/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIChain.h
 *
 *  Created on: Feb 20, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPICHAIN_H_
#define ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPICHAIN_H_

#include "adios2/toolkit/aggregator/mpi/MPIAggregator.h"

namespace adios2
{
namespace aggregator
{

class MPIChain : public MPIAggregator
{

public:
    MPIChain();

    ~MPIChain() = default;

    void Init(const size_t subStreams, MPI_Comm parentComm) final;

    std::vector<std::vector<MPI_Request>> IExchange(BufferSTL &bufferSTL,
                                                    const int step) final;

    std::vector<std::vector<MPI_Request>>
    IExchangeAbsolutePosition(BufferSTL &bufferSTL, const int step) final;

    void Wait(std::vector<std::vector<MPI_Request>> &request,
              const int step) final;

    void WaitAbsolutePosition(std::vector<std::vector<MPI_Request>> &requests,
                              const int step) final;

    void SwapBuffers(const int step) noexcept final;

    void ResetBuffers() noexcept final;

    BufferSTL &GetConsumerBuffer(BufferSTL &bufferSTL) final;

private:
    bool m_IsInExchangeAbsolutePosition = false;
    size_t m_SizeSend = 0;
    size_t m_ExchangeAbsolutePosition = 0;

    /** current sender/receiver pair order (0: sender is original buffer and
    receiver is extra buffer, 1: sender is extra buffer and receiver is the
    original buffer)*/
    unsigned int m_CurrentBufferOrder = 0;

    void HandshakeLinks();

    /**
     * Returns a reference to the sender buffer depending on
     * m_CurrentBufferOrder
     * flag
     * @param bufferSTL original buffer from serializer
     * @return reference to sender buffer
     */
    BufferSTL &GetSender(BufferSTL &bufferSTL);

    /**
     * Returns a reference to the receiver buffer depending on
     * m_CurrentBufferOrder
     * flag
     * @param bufferSTL original buffer from serializer
     * @return reference to receiver buffer
     */
    BufferSTL &GetReceiver(BufferSTL &bufferSTL);

    /**
     * Resizes and updates m_Position in a bufferSTL, used for receiving buffers
     * @param newSize new size for receiving buffer
     * @param bufferSTL to be resized
     * @param hint used in exception error message
     */
    void ResizeUpdateBufferSTL(const size_t newSize, BufferSTL &bufferSTL,
                               const std::string hint);
};

} // end namespace aggregator
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPICHAIN_H_ */
