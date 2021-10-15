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

#include <ctime>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

int BP5Writer::AsyncWriteThread_EveryoneWrites(AsyncWriteInfo *info)
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
        /* Send the token before writing so everyone can start writing asap
         */
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

    m_WriteFuture = std::async(
        std::launch::async, AsyncWriteThread_EveryoneWrites, m_AsyncWriteInfo);
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
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
