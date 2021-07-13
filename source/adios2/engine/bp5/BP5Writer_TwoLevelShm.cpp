/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.cpp
 *
 */

#include "BP5Writer.h"

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

void BP5Writer::WriteMyOwnData(format::BufferV::BufferV_iovec DataVec)
{
    m_StartDataPos = m_DataPos;
    int i = 0;
    while (DataVec[i].iov_base != NULL)
    {
        if (i == 0)
        {
            m_FileDataManager.WriteFileAt((char *)DataVec[i].iov_base,
                                          DataVec[i].iov_len, m_StartDataPos);
        }
        else
        {
            m_FileDataManager.WriteFiles((char *)DataVec[i].iov_base,
                                         DataVec[i].iov_len);
        }
        m_DataPos += DataVec[i].iov_len;
        i++;
    }
}

void BP5Writer::WriteData_TwoLevelShm(format::BufferV *Data)
{
    const aggregator::MPIShmChain *a =
        dynamic_cast<aggregator::MPIShmChain *>(m_Aggregator);

    format::BufferV::BufferV_iovec DataVec = Data->DataVec();

    // new step writing starts at offset m_DataPos on master aggregator
    // other aggregators to the same file will need to wait for the position
    // to arrive from the rank below

    // align to PAGE_SIZE (only valid on master aggregator at this point)
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos,
                                              m_Parameters.FileSystemPageSize);

    // Each aggregator needs to know the total size they write
    // including alignment to page size
    // This calculation is valid on aggregators only
    std::vector<uint64_t> mySizes = a->m_Comm.GatherValues(Data->Size());
    uint64_t myTotalSize = 0;
    uint64_t pos = m_DataPos;
    for (auto s : mySizes)
    {
        uint64_t alignment =
            helper::PaddingToAlignOffset(pos, m_Parameters.FileSystemPageSize);
        myTotalSize += alignment + s;
        pos += alignment + s;
    }

    int shmFillerToken = 0;
    if (a->m_IsAggregator)
    {
        // In each aggregator chain, send from master down the line
        // these total sizes, so every aggregator knows where to start
        if (a->m_AggregatorChainComm.Rank() > 0)
        {
            a->m_AggregatorChainComm.Recv(
                &m_DataPos, 1, a->m_AggregatorChainComm.Rank() - 1, 0,
                "AggregatorChain token in BP5Writer::WriteData_TwoLevelShm");
            // align to PAGE_SIZE
            m_DataPos += helper::PaddingToAlignOffset(
                m_DataPos, m_Parameters.FileSystemPageSize);
        }
        if (a->m_AggregatorChainComm.Rank() <
            a->m_AggregatorChainComm.Size() - 1)
        {
            uint64_t nextWriterPos = m_DataPos + myTotalSize;
            a->m_AggregatorChainComm.Isend(
                &nextWriterPos, 1, a->m_AggregatorChainComm.Rank() + 1, 0,
                "Chain token in BP5Writer::WriteData");
        }
        else if (a->m_AggregatorChainComm.Size() > 1)
        {
            // send back final position from last aggregator in file to master
            // aggregator
            uint64_t nextWriterPos = m_DataPos + myTotalSize;
            a->m_AggregatorChainComm.Isend(
                &nextWriterPos, 1, 0, 0, "Chain token in BP5Writer::WriteData");
        }
        std::cout << "Rank " << m_Comm.Rank() << " aggregator writes step "
                  << m_WriterStep << " to subfile " << a->m_SubStreamIndex
                  << " at pos " << m_DataPos << " size " << myTotalSize
                  << std::endl;
        // Send token to first non-aggregator to start filling shm
        if (a->m_Comm.Size() > 1)
        {
            a->m_Comm.Isend(&shmFillerToken, 1, a->m_Comm.Rank() + 1, 0,
                            "Shm token in BP5Writer::WriteData_TwoLevelShm");
        }
        WriteMyOwnData(DataVec);

        /* TODO Write from shm until it's over */

        // Master aggregator needs to know where the last writing ended by the
        // last aggregator in the chain, so that it can start from the correct
        // position at the next output step
        if (a->m_AggregatorChainComm.Size() > 1 &&
            !a->m_AggregatorChainComm.Rank())
        {
            a->m_AggregatorChainComm.Recv(
                &m_DataPos, 1, a->m_AggregatorChainComm.Size() - 1, 0,
                "Chain token in BP5Writer::WriteData");
        }
    }
    else
    {
        // non-aggregators fill shared buffer in marching order
        a->m_Comm.Recv(&shmFillerToken, 1, a->m_Comm.Rank() - 1, 0,
                       "Shm token in BP5Writer::WriteData_TwoLevelShm");
        std::cout << "Rank " << m_Comm.Rank()
                  << " non-aggregator recv token to fill shm " << std::endl;
        if (a->m_Comm.Rank() < a->m_Comm.Size() - 1)
        {
            a->m_Comm.Isend(&shmFillerToken, 1, a->m_Comm.Rank() + 1, 0,
                            "Shm token in BP5Writer::WriteData_TwoLevelShm");
        }
    }

    delete[] DataVec;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
