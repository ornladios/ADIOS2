/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MPIShmChain.h
 *
 *  Created on: July 5, 2021
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */
#include "MPIShmChain.h"

namespace adios2
{
namespace aggregator
{

MPIShmChain::MPIShmChain() : MPIAggregator() {}

MPIShmChain::~MPIShmChain()
{
    Close();
    /*if (m_IsActive)
    {
        m_NodeComm.Free("free per-node comm in ~MPIShmChain()");
        m_OnePerNodeComm.Free("free chain of nodes in ~MPIShmChain()");
        m_AllAggregatorsComm.Free(
            "free comm of all aggregators in ~MPIShmChain()");
        m_AggregatorChainComm.Free(
            "free chains of aggregators in ~MPIShmChain()");
    }*/
}

void MPIShmChain::Close()
{
    if (m_IsActive)
    {
        m_NodeComm.Free("free per-node comm in ~MPIShmChain()");
        m_OnePerNodeComm.Free("free chain of nodes in ~MPIShmChain()");
        m_AllAggregatorsComm.Free(
            "free comm of all aggregators in ~MPIShmChain()");
        m_AggregatorChainComm.Free(
            "free chains of aggregators in ~MPIShmChain()");
    }
    MPIAggregator::Close();
}

size_t MPIShmChain::PreInit(helper::Comm const &parentComm)
{
    /* Communicator connecting ranks on each Compute Node */
    m_NodeComm = parentComm.GroupByShm("creating per-node comm at Open");
    int NodeRank = m_NodeComm.Rank();

    /*
     *  Communicators connecting rank N of each node
     *  We are only interested in the chain of rank 0s
     */
    int color = (NodeRank ? 1 : 0);
    m_OnePerNodeComm =
        parentComm.Split(color, 0, "creating chain of nodes at Open");

    /* Number of nodes */
    if (!NodeRank)
    {
        m_NumNodes = static_cast<size_t>(m_OnePerNodeComm.Size());
    }
    m_NumNodes = m_NodeComm.BroadcastValue<size_t>(m_NumNodes, 0);
    PreInitCalled = true;
    return m_NumNodes;
}

void MPIShmChain::Init(const size_t numAggregators, const size_t subStreams,
                       helper::Comm const &parentComm)
{
    if (!PreInitCalled)
    {
        PreInit(parentComm);
    }

    // int AllRank = parentComm.Rank();
    // int AllSize = parentComm.Size();
    int NodeRank = m_NodeComm.Rank();
    size_t NodeSize = static_cast<size_t>(m_NodeComm.Size());

    /* Number of aggregators per node */
    size_t aggregatorPerNode = numAggregators / m_NumNodes;
    if (aggregatorPerNode == 0)
    {
        aggregatorPerNode = 1;
    }
    if (aggregatorPerNode > NodeSize)
    {
        aggregatorPerNode = NodeSize;
    }

    /* Create main communicator that splits the node comm into one or more
     * aggregator chains */
    float k =
        static_cast<float>(NodeSize) / static_cast<float>(aggregatorPerNode);
    float c = static_cast<float>(NodeRank) / k;
    int color = static_cast<int>(c);
    m_Comm = m_NodeComm.Split(color, 0, "creating aggregator groups at Open");
    m_Rank = m_Comm.Rank();
    m_Size = m_Comm.Size();
    if (m_Rank != 0)
    {
        m_IsAggregator = false;
        m_IsMasterAggregator = false;
    }

    /* Identify parent rank of aggregator process within each chain */
    if (!m_Rank)
    {
        m_AggregatorRank = parentComm.Rank();
    }
    m_AggregatorRank = m_Comm.BroadcastValue<int>(m_AggregatorRank, 0);

    /* Communicator for all Aggregators */
    color = (m_Rank ? 1 : 0);
    m_AllAggregatorsComm =
        parentComm.Split(color, 0, "creating comm of all aggregators at Open");

    /* Total number of aggregators */
    if (!NodeRank)
    {
        m_NumAggregators = static_cast<size_t>(m_AllAggregatorsComm.Size());
    }
    m_NumAggregators = m_NodeComm.BroadcastValue<size_t>(m_NumAggregators);

    /* Number of substreams */
    m_SubStreams = subStreams;
    if (m_SubStreams == 0)
    {
        m_SubStreams = 1;
    }
    if (m_SubStreams > m_NumAggregators)
    {
        m_SubStreams = m_NumAggregators;
    }

    if (!m_Rank)
    {
        k = static_cast<float>(m_NumAggregators) /
            static_cast<float>(m_SubStreams);
        /* 1.0 <= k <= m_NumAggregators */
        c = static_cast<float>(m_AllAggregatorsComm.Rank()) / k;
        m_SubStreamIndex = static_cast<int>(c);
    }
    m_SubStreamIndex = m_Comm.BroadcastValue<size_t>(m_SubStreamIndex);

    /* Create the communicator to connect aggregators writing to the same
     * substream */
    color = m_SubStreamIndex;
    m_AggregatorChainComm = m_AllAggregatorsComm.Split(
        color, 0, "creating chains of aggregators at Open");

    if (m_AggregatorChainComm.Rank() != 0)
    {
        m_IsMasterAggregator = false;
    }

    m_IsActive = true;

    HandshakeLinks();
}

// PRIVATE
void MPIShmChain::HandshakeLinks()
{
    int link = -1;

    helper::Comm::Req sendRequest;
    if (m_Rank > 0) // send
    {
        sendRequest = m_Comm.Isend(
            &m_Rank, 1, m_Rank - 1, 0,
            "Isend handshake with neighbor, MPIChain aggregator, at Open");
    }

    if (m_Rank < m_Size - 1) // receive
    {
        helper::Comm::Req receiveRequest = m_Comm.Irecv(
            &link, 1, m_Rank + 1, 0,
            "Irecv handshake with neighbor, MPIChain aggregator, at Open");

        receiveRequest.Wait("Irecv Wait handshake with neighbor, MPIChain "
                            "aggregator, at Open");
    }

    if (m_Rank > 0)
    {
        sendRequest.Wait("Isend wait handshake with neighbor, MPIChain "
                         "aggregator, at Open");
    }
}

} // end namespace aggregator
} // end namespace adios2
