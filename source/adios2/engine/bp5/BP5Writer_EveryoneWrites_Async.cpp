/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.cpp
 *
 */

#include "BP5Writer.h"
#include "BP5Writer.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include <adios2-perfstubs-interface.h>

#include <algorithm> // max
#include <ctime>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

int BP5Writer::AsyncWriteThread_EveryoneWrites_Throttled(AsyncWriteInfo *info)
{
    Seconds ts = Now() - info->tstart;
    // std::cout << "ASYNC starts at: " << ts.count() << std::endl;
    std::vector<core::iovec> DataVec = info->Data->DataVec();

    double deadline = info->deadline;
    if (info->tokenChain)
    {
        if (info->rank_chain > 0)
        {
            info->tokenChain->RecvToken();
        }
        Seconds elapsed = Now() - info->tstart - ts;
        deadline -= elapsed.count();
        int remprocs = info->nproc_chain - info->rank_chain;
        deadline = deadline / remprocs;
    }

    if (deadline < 0.0)
    {
        deadline = 0.0;
    }

    info->tm->WriteFileAt(DataVec.data(), DataVec.size(), info->Data->Size(),
                          info->startPos, deadline, info->flagRush);
    if (info->tokenChain)
    {
        uint64_t t = 1;
        info->tokenChain->SendToken(t);
        if (!info->rank_chain)
        {
            info->tokenChain->RecvToken();
        }
    }
    delete info->Data;
    ts = Now() - info->tstart;
    // std::cout << "ASYNC ended at: " << ts.count() << std::endl;
    return 1;
};

int BP5Writer::AsyncWriteThread_EveryoneWrites_Guided(AsyncWriteInfo *info)
{
    core::TimePoint starttime = core::Now();
    std::vector<core::iovec> DataVec = info->Data->DataVec();
    size_t totalsize = info->Data->Size();
    double deadline = info->deadline;

    if (info->tokenChain)
    {
        if (info->rank_chain > 0)
        {
            info->tokenChain->RecvToken();
        }
        Seconds elapsed = Now() - starttime;
        deadline -= elapsed.count();
        if (deadline < 0.0)
        {
            deadline = 0.0;
        }
        int remprocs = info->nproc_chain - info->rank_chain;
        deadline = deadline / remprocs;
        starttime = core::Now(); // this process starts now
    }

    core::Seconds deadlineSeconds = core::Seconds(deadline);

    /* local variables to track variables modified by main thread */
    size_t compBlockID = 0;  /* which computation block we are in */
    size_t compBlockIdx = 0; /* position in vector to get length */
    size_t nExpectedBlocks = info->expectedComputationBlocks.size();
    bool inComp = false;

    /* In a loop, write the data in smaller blocks */
    size_t nBlocks = DataVec.size();
    size_t wrote = 0;
    size_t block = 0;
    size_t temp_offset = 0;
    size_t max_size = std::max(1024 * 1024UL, totalsize / 100UL);

    bool firstWrite = true;
    while (block < nBlocks)
    {
        if (*info->flagRush)
        {
            max_size = MaxSizeT;
        }
        else
        {
            core::Seconds timesofar = core::Now() - starttime;
            /*std::cout << "  Wrote = " << wrote
                      << " time so far = " << timesofar.count() << std::endl;*/
            if (timesofar > deadlineSeconds)
            {
                // Passed the deadline, write the rest without any waiting
                *info->flagRush = true; // this thread can rush it too
                max_size = MaxSizeT;
            }
        }

        /* Get the next n bytes from the current block, current offset */
        size_t n = DataVec[block].iov_len - temp_offset;
        if (n > max_size)
        {
            n = max_size;
        }

        if (!*info->flagRush && compBlockIdx < nExpectedBlocks)
        {
            info->lock->lock();
            // access variables modified by main thread
            // (the ones that cause race conditions)
            compBlockID = *info->currentComputationBlockID;
            inComp = *info->inComputationBlock;
            info->lock->unlock();
        }
        else
        {
            inComp = false;
        }

        /* Track which computation block we are in */
        if (inComp)
        {
            while (compBlockIdx < nExpectedBlocks &&
                   info->expectedComputationBlocks[compBlockIdx].blockID <
                       compBlockID)
            {
                ++compBlockIdx;
            }
            if (info->expectedComputationBlocks[compBlockIdx].blockID >
                compBlockID)
            {
                // the current computation block is a short one that was not
                // recorded
                inComp = false;
            }
        }

        /* Scheduling decisions:
           Cases:
           1. Not in a computation block AND we still expect more computation
           blocks down the line ==> Sleep
           2. In computation block ==> Write
           3. We are at the end of a computation block (how close??) AND we
           still expect more computation blocks down the line 3. ==> Sleep
           4. We are at the end of the LAST computation block ==> Write
           5. No more computation blocks expected ==> throttled Write to
           deadline
           6. Main thread set flagRush ==> Write
        */

        bool doSleep = false;
        bool doThrottle = false;
        // cases 2, 4 and 6 need no more settings
        if (!*info->flagRush)
        {
            if (!inComp && compBlockIdx < nExpectedBlocks)
            {
                // case 1
                doSleep = true;
            }
            if (inComp && false)
            {
                // case 3 not handled yet
                doSleep = true;
            }
            if (!inComp && compBlockIdx >= nExpectedBlocks)
            {
                // case 5
                doThrottle = true;
            }
        }

        if (doSleep)
        {
            std::this_thread::sleep_for(core::Seconds(0.01));
            continue;
        }

        if (doThrottle)
        {
            // Write the rest of data in throttled manner within the available
            // deadline
            core::Seconds timesofar = core::Now() - starttime;
            double finaldeadline = deadline - timesofar.count();
            if (finaldeadline < 0.0)
            {
                finaldeadline = 0.0;
            }

            auto vec = std::vector<adios2::core::iovec>(DataVec.begin() + block,
                                                        DataVec.end());
            vec[0].iov_base =
                (const char *)DataVec[block].iov_base + temp_offset;
            vec[0].iov_len = n;

            std::cout << "Async write on Rank " << info->rank_global
                      << " throttle-write the last " << totalsize - wrote
                      << " bytes with deadline " << finaldeadline << " sec"
                      << std::endl;

            info->tm->WriteFileAt(vec.data(), vec.size(), totalsize - wrote,
                                  info->startPos + wrote, finaldeadline,
                                  info->flagRush);
            break;
        }

        /* Write now */
        if (firstWrite)
        {
            info->tm->WriteFileAt((const char *)DataVec[block].iov_base +
                                      temp_offset,
                                  n, info->startPos);
            firstWrite = false;
        }
        else
        {
            info->tm->WriteFiles(
                (const char *)DataVec[block].iov_base + temp_offset, n);
        }

        /* Have we processed the entire block or staying with it? */
        if (n + temp_offset < DataVec[block].iov_len)
        {
            temp_offset += n;
        }
        else
        {
            temp_offset = 0;
            ++block;
        }
        wrote += n;
    }

    if (info->tokenChain)
    {
        uint64_t t = 1;
        info->tokenChain->SendToken(t);
        if (!info->rank_chain)
        {
            info->tokenChain->RecvToken();
        }
    }
    delete info->Data;
    return 1;
};

void BP5Writer::WriteData_EveryoneWrites_Async(format::BufferV *Data,
                                               bool SerializedWriters)
{

    const aggregator::MPIChain *a =
        dynamic_cast<aggregator::MPIChain *>(m_Aggregator);

    // new step writing starts at offset m_DataPos on aggregator
    // others will wait for the position to arrive from the rank below

    if (a->m_Comm.Rank() > 0)
    {
        a->m_Comm.Recv(
            &m_DataPos, 1, a->m_Comm.Rank() - 1, 0,
            "Chain token in BP5Writer::WriteData_EveryoneWrites_Async");
    }

    // align to PAGE_SIZE
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos,
                                              m_Parameters.FileSystemPageSize);
    m_StartDataPos = m_DataPos;

    if (a->m_Comm.Rank() < a->m_Comm.Size() - 1)
    {
        uint64_t nextWriterPos = m_DataPos + Data->Size();
        a->m_Comm.Isend(
            &nextWriterPos, 1, a->m_Comm.Rank() + 1, 0,
            "Chain token in BP5Writer::WriteData_EveryoneWrites_Async");
    }

    m_DataPos += Data->Size();

    /* a->comm can span multiple nodes but we need comm inside a node
       when doing serialized aggregation */
    m_AsyncWriteInfo = new AsyncWriteInfo();
    m_AsyncWriteInfo->aggregator = nullptr;
    m_AsyncWriteInfo->rank_global = m_Comm.Rank();
    if (SerializedWriters)
    {
        m_AsyncWriteInfo->comm_chain = a->m_Comm.GroupByShm();
        m_AsyncWriteInfo->rank_chain = m_AsyncWriteInfo->comm_chain.Rank();
        m_AsyncWriteInfo->nproc_chain = m_AsyncWriteInfo->comm_chain.Size();
        m_AsyncWriteInfo->tokenChain =
            new shm::TokenChain<uint64_t>(&m_AsyncWriteInfo->comm_chain);
    }
    else
    {
        m_AsyncWriteInfo->comm_chain = helper::Comm(); // not needed
        m_AsyncWriteInfo->rank_chain = a->m_Comm.Rank();
        m_AsyncWriteInfo->nproc_chain = a->m_Comm.Size();
        m_AsyncWriteInfo->tokenChain = nullptr;
    }
    m_AsyncWriteInfo->tstart = m_EngineStart;
    m_AsyncWriteInfo->tm = &m_FileDataManager;
    m_AsyncWriteInfo->Data = Data;
    m_AsyncWriteInfo->startPos = m_StartDataPos;
    m_AsyncWriteInfo->totalSize = Data->Size();
    m_AsyncWriteInfo->deadline = m_ExpectedTimeBetweenSteps.count();
    m_AsyncWriteInfo->flagRush = &m_flagRush;
    m_AsyncWriteInfo->lock = &m_AsyncWriteLock;

    if (m_ComputationBlocksLength > 0.0 &&
        m_Parameters.AsyncWrite == (int)AsyncWrite::Guided)
    {
        m_AsyncWriteInfo->inComputationBlock = &m_InComputationBlock;
        m_AsyncWriteInfo->computationBlocksLength = m_ComputationBlocksLength;
        if (m_AsyncWriteInfo->deadline < m_ComputationBlocksLength)
        {
            m_AsyncWriteInfo->deadline = m_ComputationBlocksLength;
        }
        m_AsyncWriteInfo->expectedComputationBlocks =
            m_ComputationBlockTimes; // copy!
        m_AsyncWriteInfo->currentComputationBlocks =
            &m_ComputationBlockTimes; // ptr!
        m_AsyncWriteInfo->currentComputationBlockID = &m_ComputationBlockID;

        /* Clear current block tracker now so that async thread does not get
        confused with the past info */
        m_ComputationBlockTimes.clear();
        m_ComputationBlocksLength = 0.0;
        m_ComputationBlockID = 0;
        m_WriteFuture = std::async(std::launch::async,
                                   AsyncWriteThread_EveryoneWrites_Guided,
                                   m_AsyncWriteInfo);
    }
    else
    {
        if (m_Parameters.AsyncWrite == (int)AsyncWrite::Naive)
        {
            m_AsyncWriteInfo->deadline = 0;
        }
        m_AsyncWriteInfo->inComputationBlock = nullptr;
        m_AsyncWriteInfo->computationBlocksLength = 0.0;
        m_AsyncWriteInfo->currentComputationBlockID = nullptr;
        m_WriteFuture = std::async(std::launch::async,
                                   AsyncWriteThread_EveryoneWrites_Throttled,
                                   m_AsyncWriteInfo);
    }
    // At this point modifying Data in main thread is prohibited !!!

    if (a->m_Comm.Size() > 1)
    {
        // at the end, last rank sends back the final data pos to first rank
        // so it can update its data pos
        if (a->m_Comm.Rank() == a->m_Comm.Size() - 1)
        {
            a->m_Comm.Isend(&m_DataPos, 1, 0, 0,
                            "Final chain token in "
                            "BP5Writer::WriteData_EveryoneWrites_Async");
        }
        if (a->m_Comm.Rank() == 0)
        {
            a->m_Comm.Recv(
                &m_DataPos, 1, a->m_Comm.Size() - 1, 0,
                "Chain token in BP5Writer::WriteData_EveryoneWrites_Async");
        }
    }
}

void BP5Writer::AsyncWriteDataCleanup_EveryoneWrites()
{
    if (m_AsyncWriteInfo->tokenChain)
    {
        delete m_AsyncWriteInfo->tokenChain;
    }
    delete m_AsyncWriteInfo;
    m_AsyncWriteInfo = nullptr;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
