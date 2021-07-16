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

#ifndef ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPISHMCHAIN_H_
#define ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPISHMCHAIN_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/toolkit/aggregator/mpi/MPIAggregator.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace adios2
{
namespace aggregator
{

class Spinlock
{
    /* from
     * https://wang-yimu.com/a-tutorial-on-shared-memory-inter-process-communication
     */
public:
    Spinlock() { flag_.clear(); }
    void lock()
    {
        while (!try_lock())
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(0.00001));
        }
    }
    void unlock() { flag_.clear(); }

private:
    inline bool try_lock() { return !flag_.test_and_set(); }
    std::atomic_flag flag_; //{ATOMIC_FLAG_INIT};
};

constexpr size_t SHM_BUF_SIZE = 4194304; // 4MB
// we allocate 2x this size + a bit for shared memory segment

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

    void Close() final;

    /**
     * true: the Master (aggregator) process in the chain
     * always m_Rank == m_Comm.Rank() == 0 for a master aggregator
     * same as (m_AggregatorChainComm.Rank() == 0)
     */
    bool m_IsMasterAggregator = true;

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

    struct ShmDataBuffer
    {
        size_t max_size;    // max size for buf
        size_t actual_size; // size of actual content
        // points to data buffer in shared memory
        // Warning: This is a different address on every process
        char *buf;
    };

    ShmDataBuffer *LockProducerBuffer();
    void UnlockProducerBuffer();
    ShmDataBuffer *LockConsumerBuffer();
    void UnlockConsumerBuffer();
    void ResetBuffers() noexcept;

private:
    helper::Comm::Req HandshakeLinks_Start();
    void HandshakeLinks_Complete(helper::Comm::Req &req);

    helper::Comm::Win m_Win;
    void CreateShm();
    void DestroyShm();

    enum class LastBufferUsed
    {
        None,
        A,
        B
    };

    struct ShmSegment
    {
        LastBufferUsed producerBuffer;
        LastBufferUsed consumerBuffer;
        unsigned int NumBuffersFull;
        // user facing structs
        ShmDataBuffer sdbA;
        ShmDataBuffer sdbB;
        // locks for individual buffers (sdb and buf)
        aggregator::Spinlock lockA;
        aggregator::Spinlock lockB;
        // the actual data buffers
        char bufA[SHM_BUF_SIZE];
        char bufB[SHM_BUF_SIZE];
    };
    ShmSegment *m_Shm;
};

} // end namespace aggregator
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_AGGREGATOR_MPI_MPISHMCHAIN_H_ */
