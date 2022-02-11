/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferV.cpp
 *
 */

#include "MallocV.h"
#include "adios2/toolkit/format/buffer/BufferV.h"

#include <algorithm>
#include <assert.h>
#include <stddef.h> // max_align_t
#include <string.h>

namespace adios2
{
namespace format
{

MallocV::MallocV(const std::string type, const bool AlwaysCopy,
                 const size_t MemAlign, const size_t MemBlockSize,
                 size_t InitialBufferSize, double GrowthFactor)
: BufferV(type, AlwaysCopy, MemAlign, MemBlockSize),
  m_InitialBufferSize(InitialBufferSize), m_GrowthFactor(GrowthFactor)
{
}

MallocV::~MallocV()
{
    if (m_InternalBlock)
        free(m_InternalBlock);
}

void MallocV::Reset()
{
    CurOffset = 0;
    m_internalPos = 0;
    DataV.clear();
}

/*
 *  This is used in PerformPuts() to copy externally referenced data
 *  so that it can be modified by the application. It does *not*
 *  change the metadata offset that was originally returned by
 *  AddToVec.  That is, it relocates the data from application memory
 *  into the internal buffer, but it does not change the position of
 *  that data in the write order, which may result in non-contiguous
 *  writes from the internal buffer.
 */
void MallocV::CopyExternalToInternal()
{
    for (std::size_t i = 0; i < DataV.size(); ++i)
    {
        if (DataV[i].External)
        {
            size_t size = DataV[i].Size;

            /* force internal buffer alignment */
            (void)AddToVec(0, NULL, sizeof(max_align_t), true);

            if (m_internalPos + size > m_AllocatedSize)
            {
                // need to resize
                size_t NewSize;
                if (m_internalPos + size > m_AllocatedSize * m_GrowthFactor)
                {
                    // just grow as needed (more than GrowthFactor)
                    NewSize = m_internalPos + size;
                }
                else
                {
                    NewSize = (size_t)(m_AllocatedSize * m_GrowthFactor);
                }
                m_InternalBlock = (char *)realloc(m_InternalBlock, NewSize);
                m_AllocatedSize = NewSize;
            }
            memcpy(m_InternalBlock + m_internalPos, DataV[i].Base, size);
            DataV[i].External = false;
            DataV[i].Base = NULL;
            DataV[i].Offset = m_internalPos;
            m_internalPos += size;
        }
    }
}

size_t MallocV::AddToVec(const size_t size, const void *buf, size_t align,
                         bool CopyReqd, MemorySpace MemSpace)
{
    AlignBuffer(align); // may call back AddToVec recursively
    size_t retOffset = CurOffset;

    if (size == 0)
    {
        return CurOffset;
    }

    if (!CopyReqd && !m_AlwaysCopy)
    {
        // just add buf to internal version of output vector
        VecEntry entry = {true, buf, 0, size};
        DataV.push_back(entry);
    }
    else
    {
        if (m_internalPos + size > m_AllocatedSize)
        {
            // need to resize
            size_t NewSize;
            if (m_internalPos + size > m_AllocatedSize * m_GrowthFactor)
            {
                // just grow as needed (more than GrowthFactor)
                NewSize = m_internalPos + size;
            }
            else
            {
                NewSize = (size_t)(m_AllocatedSize * m_GrowthFactor);
            }
            m_InternalBlock = (char *)realloc(m_InternalBlock, NewSize);
            m_AllocatedSize = NewSize;
        }
        memcpy(m_InternalBlock + m_internalPos, buf, size);

        if (DataV.size() && !DataV.back().External &&
            (m_internalPos == (DataV.back().Offset + DataV.back().Size)))
        {
            // just add to the size of the existing tail entry
            DataV.back().Size += size;
        }
        else
        {
            DataV.push_back({false, NULL, m_internalPos, size});
        }
        m_internalPos += size;
    }
    CurOffset = retOffset + size;
    return retOffset;
}

BufferV::BufferPos MallocV::Allocate(const size_t size, size_t align)
{
    if (size == 0)
    {
        return BufferPos(-1, 0, CurOffset);
    }

    AlignBuffer(align);

    if (m_internalPos + size > m_AllocatedSize)
    {
        // need to resize
        size_t NewSize;
        if (m_internalPos + size > m_AllocatedSize * m_GrowthFactor)
        {
            // just grow as needed (more than GrowthFactor)
            NewSize = m_internalPos + size;
        }
        else
        {
            NewSize = (size_t)(m_AllocatedSize * m_GrowthFactor);
        }
        m_InternalBlock = (char *)realloc(m_InternalBlock, NewSize);
        m_AllocatedSize = NewSize;
    }

    if (DataV.size() && !DataV.back().External &&
        (m_internalPos == (DataV.back().Offset + DataV.back().Size)))
    {
        // just add to the size of the existing tail entry
        DataV.back().Size += size;
    }
    else
    {
        DataV.push_back({false, NULL, m_internalPos, size});
    }

    BufferPos bp(0, m_internalPos, CurOffset);
    // valid ptr anytime <-- m_InternalBlock + m_internalPos;

    CurOffset += size;
    m_internalPos += size;

    return bp;
}

void MallocV::DownsizeLastAlloc(const size_t oldSize, const size_t newSize)
{
    DataV.back().Size -= (oldSize - newSize);
    CurOffset -= (oldSize - newSize);
    m_internalPos -= (oldSize - newSize);
}

void *MallocV::GetPtr(int bufferIdx, size_t posInBuffer)
{
    if (bufferIdx == -1)
    {
        return nullptr;
    }
    else
    {
        return m_InternalBlock + posInBuffer;
    }
}

std::vector<core::iovec> MallocV::DataVec() noexcept
{
    std::vector<core::iovec> iov(DataV.size());
    for (std::size_t i = 0; i < DataV.size(); ++i)
    {
        if (DataV[i].External)
        {
            iov[i].iov_base = DataV[i].Base;
        }
        else
        {
            iov[i].iov_base = m_InternalBlock + DataV[i].Offset;
        }
        iov[i].iov_len = DataV[i].Size;
    }
    return iov;
}

} // end namespace format
} // end namespace adios2
