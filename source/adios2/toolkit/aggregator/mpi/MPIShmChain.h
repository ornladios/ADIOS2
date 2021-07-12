/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIShmChain.h
 *
 *  Created on: July 5, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 *
 */

#ifndef ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPICSHMHAIN_H_
#define ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPISHMCHAIN_H_

#include "adios2/toolkit/aggregator/mpi/MPIAggregator.h"

namespace adios2
{
namespace aggregator
{

/** A one- or two-layer aggregator chain for using Shared memory within a
 * compute node.
 * Use MPI split type to group processes within a node into one chain.
 * Depending on the number of aggregators, multiple nodes may be merged into
 * a two-layer chain, (multiple Aggregators and one Master).
 * Otherwise its a simple chain with one Aggregator=Master.
 *
 * m_Comm is the communicator that split for each node
 *
 */

class MPIShmChain : public MPIAggregator
{

public:
    MPIShmChain();

    ~MPIShmChain();

    /* Create a per-node communicator and return number of nodes */
    size_t PreInit(helper::Comm const &parentComm);

    void Init(const size_t numAggregators, const size_t subStreams,
              helper::Comm const &parentComm) final;

    /**
     * true: the Master (aggregator) process in the chain
     * always m_Rank == m_Comm.Rank() == 0 for a master aggregator
     * same as (m_AggregatorChainComm.Rank() == 0)
     */
    bool m_IsMasterAggregator = true;

    /* These are not used and must be removed */
    ExchangeRequests IExchange(format::Buffer &buffer, const int step) final;

    ExchangeAbsolutePositionRequests
    IExchangeAbsolutePosition(format::Buffer &buffer, const int step) final;

    void Wait(ExchangeRequests &requests, const int step) final;

    void WaitAbsolutePosition(ExchangeAbsolutePositionRequests &requests,
                              const int step) final;

private:
    void HandshakeLinks();

    /*
        Variables set in PreInit
    */

    bool PreInitCalled = false;
    /* Communicator per compute node */
    helper::Comm m_NodeComm;
    /* Communicator connecting rank 0 on each node
       Useful only on rank 0s of m_NodeComm */
    helper::Comm m_OnePerNodeComm;
    /* Number of Compute Nodes
     * (size of m_OnePerNodeComm created from rank 0s of m_NodeComm)
     */
    size_t m_NumNodes;

    /*
        Variables set in Init
    */

    /* Communicator connecting all aggregators
        (rank 0 of each aggregator group)
        Useful only on aggregators themselves
    */
    helper::Comm m_AllAggregatorsComm;

    /* Communicator connecting the aggregators
       that write to the same substream.
       rank 0 becomes a MasterAggregator
       Useful only on aggregators themselves
       */
    helper::Comm m_AggregatorChainComm;
};

} // end namespace aggregator
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPISHMCHAIN_H_ */
