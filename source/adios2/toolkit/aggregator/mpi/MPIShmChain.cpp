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

#include <iostream>

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
        aggregatorPerNode = 1; /* default */
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
        m_SubStreams = m_NumAggregators; /* default */
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

    helper::Comm::Req sendRequest = HandshakeLinks_Start();

    /* Create the shared memory segment */
    if (m_Comm.Size() > 1)
    {
        CreateShm();
    }

    HandshakeLinks_Complete(sendRequest);
}

// PRIVATE
helper::Comm::Req MPIShmChain::HandshakeLinks_Start()
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
    return sendRequest;
}

void MPIShmChain::HandshakeLinks_Complete(helper::Comm::Req &req)
{
    if (m_Rank > 0)
    {
        req.Wait("Isend wait handshake with neighbor, MPIChain "
                 "aggregator, at Open");
    }
}

void MPIShmChain::CreateShm()
{
    void *ptr;
    if (!m_Rank)
    {
        m_Win = m_Comm.Win_allocate_shared(sizeof(ShmSegment), 1, &ptr);
    }

    if (m_Rank)
    {
        m_Win = m_Comm.Win_allocate_shared(0, 1, &ptr);
        size_t shmsize;
        int disp_unit;
        m_Comm.Win_shared_query(m_Win, 0, &shmsize, &disp_unit, &ptr);
    }
    m_Shm = reinterpret_cast<ShmSegment *>(ptr);
    m_Shm->producerBuffer = BufferUse::None;
    m_Shm->consumerBuffer = BufferUse::None;
    m_Shm->sdbA.buf = nullptr;
    m_Shm->sdbA.max_size = SHM_BUF_SIZE;
    m_Shm->sdbB.buf = nullptr;
    m_Shm->sdbB.max_size = SHM_BUF_SIZE;

    std::cout << "Rank " << m_Rank << " shm = " << ptr
              << " bufA = " << static_cast<void *>(m_Shm->bufA)
              << " bufB = " << static_cast<void *>(m_Shm->bufB) << std::endl;
}

void MPIShmChain::DestroyShm() { m_Comm.Win_free(m_Win); }

MPIShmChain::ShmDataBuffer *MPIShmChain::LockProducerBuffer()
{
    MPIShmChain::ShmDataBuffer *sdb = nullptr;

    m_Comm.Win_Lock(helper::Comm::LockType::Exclusive, 0, 0, m_Win);
    if (m_Shm->producerBuffer == BufferUse::A)

    {
        m_Shm->producerBuffer = BufferUse::B;
        sdb = &m_Shm->sdbB;
        // point to shm data buffer (in local process memory)
        sdb->buf = m_Shm->bufB;
    }
    else // (m_Shm->producerBuffer == BufferUse::None ||
         // m_Shm->producerBuffer == BufferUse::B)
    {
        m_Shm->producerBuffer = BufferUse::A;
        sdb = &m_Shm->sdbA;
        // point to shm data buffer (in local process memory)
        sdb->buf = m_Shm->bufA;
    }
    m_Comm.Win_Unlock(0, m_Win);

    // We determined we want a specific buffer
    // Now we need to get a lock on it in case consumer is using it
    if (m_Shm->producerBuffer == BufferUse::A)
    {
        m_Shm->lockA.lock();
    }
    else
    {
        m_Shm->lockB.lock();
    }

    return sdb;
}

void MPIShmChain::UnlockProducerBuffer()
{
    if (m_Shm->producerBuffer == BufferUse::A)
    {
        m_Shm->lockA.unlock();
    }
    else
    {
        m_Shm->lockB.unlock();
    }
}

MPIShmChain::ShmDataBuffer *MPIShmChain::LockConsumerBuffer()
{
    MPIShmChain::ShmDataBuffer *sdb = nullptr;

    // Sleep until the very first production has started:
    while (m_Shm->producerBuffer == BufferUse::None)
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(0.00001));
    }
    // At this point we know buffer A has content or going to have content
    // when we successfully lock it

    m_Comm.Win_Lock(helper::Comm::LockType::Exclusive, 0, 0, m_Win);
    if (m_Shm->consumerBuffer == BufferUse::A)

    {
        m_Shm->consumerBuffer = BufferUse::B;
        sdb = &m_Shm->sdbB;
        // point to shm data buffer (in local process memory)
        sdb->buf = m_Shm->bufB;
    }
    else // (m_Shm->consumerBuffer == BufferUse::None ||
         // m_Shm->consumerBuffer == BufferUse::B)
    {
        m_Shm->consumerBuffer = BufferUse::A;
        sdb = &m_Shm->sdbA;
        // point to shm data buffer (in local process memory)
        sdb->buf = m_Shm->bufA;
    }
    m_Comm.Win_Unlock(0, m_Win);

    // We determined we want a specific buffer
    // Now we need to get a lock on it in case producer is using it
    if (m_Shm->consumerBuffer == BufferUse::A)
    {
        m_Shm->lockA.lock();
    }
    else
    {
        m_Shm->lockB.lock();
    }

    return sdb;
}

void MPIShmChain::UnlockConsumerBuffer()
{
    if (m_Shm->consumerBuffer == BufferUse::A)
    {
        m_Shm->lockA.unlock();
    }
    else
    {
        m_Shm->lockB.unlock();
    }
}

} // end namespace aggregator
} // end namespace adios2
