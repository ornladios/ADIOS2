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

    void Init(const size_t subStreams, helper::Comm const &parentComm) final;

    ExchangeRequests IExchange(format::Buffer &buffer, const int step) final;

    ExchangeAbsolutePositionRequests
    IExchangeAbsolutePosition(format::Buffer &buffer, const int step) final;

    void Wait(ExchangeRequests &requests, const int step) final;

    void WaitAbsolutePosition(ExchangeAbsolutePositionRequests &requests,
                              const int step) final;

    void SwapBuffers(const int step) noexcept final;

    void ResetBuffers() noexcept final;

    format::Buffer &GetConsumerBuffer(format::Buffer &buffer) final;

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
     * @param buffer original buffer from serializer
     * @return reference to sender buffer
     */
    format::Buffer &GetSender(format::Buffer &buffer);

    /**
     * Returns a reference to the receiver buffer depending on
     * m_CurrentBufferOrder
     * flag
     * @param buffer original buffer from serializer
     * @return reference to receiver buffer
     */
    format::Buffer &GetReceiver(format::Buffer &buffer);

    /**
     * Resizes and updates m_Position in a buffer, used for receiving buffers
     * @param newSize new size for receiving buffer
     * @param buffer to be resized
     * @param hint used in exception error message
     */
    void ResizeUpdateBuffer(const size_t newSize, format::Buffer &buffer,
                            const std::string hint);
};

} // end namespace aggregator
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPICHAIN_H_ */
