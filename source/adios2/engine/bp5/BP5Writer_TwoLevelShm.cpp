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

void BP5Writer::WriteData_TwoLevelShm(format::BufferV::BufferV_iovec DataVec)
{
    const aggregator::MPIShmChain *a =
        dynamic_cast<aggregator::MPIShmChain *>(m_Aggregator);
    ;
    // new step writing starts at offset m_DataPos on aggregator
    // others will wait for the position to arrive from the rank below

    if (a->m_Comm.Rank() > 0)
    {
        a->m_Comm.Recv(&m_DataPos, 1, a->m_Comm.Rank() - 1, 0,
                       "Chain token in BP5Writer::WriteData");
    }
    // align to PAGE_SIZE
    m_DataPos += m_Parameters.FileSystemPageSize -
                 (m_DataPos % m_Parameters.FileSystemPageSize);
    m_StartDataPos = m_DataPos;

    if (a->m_Comm.Rank() < a->m_Comm.Size() - 1)
    {
        int i = 0;
        uint64_t nextWriterPos = m_DataPos;
        while (DataVec[i].iov_base != NULL)
        {
            nextWriterPos += DataVec[i].iov_len;
            i++;
        }
        a->m_Comm.Isend(&nextWriterPos, 1, a->m_Comm.Rank() + 1, 0,
                        "Chain token in BP5Writer::WriteData");
    }

    /* Aggregator starts with writing its own data */
    if (a->m_Comm.Rank() == 0)
    {
        WriteMyOwnData(DataVec);
    }

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

    if (a->m_Comm.Size() > 1)
    {
        // at the end, last rank sends back the final data pos to first rank
        // so it can update its data pos
        if (a->m_Comm.Rank() == a->m_Comm.Size() - 1)
        {
            a->m_Comm.Isend(&m_DataPos, 1, 0, 0,
                            "Final chain token in BP5Writer::WriteData");
        }
        if (a->m_Comm.Rank() == 0)
        {
            a->m_Comm.Recv(&m_DataPos, 1, a->m_Comm.Size() - 1, 0,
                           "Chain token in BP5Writer::WriteData");
        }
    }
    delete[] DataVec;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
