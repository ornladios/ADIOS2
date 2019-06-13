/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIAggregator.h
 *
 *  Created on: Feb 20, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPIAGGREGATOR_H_
#define ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPIAGGREGATOR_H_

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/toolkit/format/BufferSTL.h"

namespace adios2
{
namespace aggregator
{

class MPIAggregator
{
public:
    /** total number of substreams */
    size_t m_SubStreams = 0;

    /** current substream index from 0 to m_SubStreams-1 */
    size_t m_SubStreamIndex = 0;

    /** split Communicator for a substream: producers and consumer (rank=0) */
    MPI_Comm m_Comm;

    /** rank from m_Comm */
    int m_Rank = 0;

    /** size from m_Comm */
    int m_Size = 1;

    /**
     * true: consumes data from itself or other processes and interacts with
     * transport managers
     */
    bool m_IsConsumer = true;

    /** true: doing aggregation, false: not doing aggregation */
    bool m_IsActive = false;

    /** consumer original rank coming from the parent communicator in Init,
     *  corresponds to m_Rank = 0 */
    int m_ConsumerRank = -1;

    MPIAggregator();

    virtual ~MPIAggregator();

    virtual void Init(const size_t subStreams, MPI_Comm parentComm);

    virtual std::vector<std::vector<MPI_Request>>
    IExchange(BufferSTL &bufferSTL, const int step) = 0;

    virtual std::vector<std::vector<MPI_Request>>
    IExchangeAbsolutePosition(BufferSTL &bufferSTL, const int step) = 0;

    virtual void
    WaitAbsolutePosition(std::vector<std::vector<MPI_Request>> &requests,
                         const int step) = 0;

    virtual void Wait(std::vector<std::vector<MPI_Request>> &requests,
                      const int step) = 0;

    virtual void SwapBuffers(const int step) noexcept;

    virtual void ResetBuffers() noexcept;

    virtual BufferSTL &GetConsumerBuffer(BufferSTL &bufferSTL);

    /** closes current aggregator, frees m_Comm */
    void Close();

protected:
    /** Init m_Comm splitting assigning ranks to subStreams (balanced except for
     * the last rank) */
    void InitComm(const size_t subStreams, MPI_Comm parentComm);

    /** handshakes a single rank with the rest of the m_Comm ranks */
    void HandshakeRank(const int rank = 0);

    /** assigning extra buffers for aggregation */
    std::vector<BufferSTL> m_Buffers;
};

} // end namespace aggregator
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_AGGREGATOR_MPIAGGREGATOR_H_ */
