/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferV.cpp
 *
 */

#include "ChunkV.h"
#include "adios2/toolkit/format/buffer/BufferV.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stddef.h> // max_align_t
#include <string.h>

namespace adios2
{
namespace format
{

ChunkV::ChunkV(const std::string type, const bool AlwaysCopy,
               const size_t ChunkSize)
: BufferV(type, AlwaysCopy), m_ChunkSize(ChunkSize)
{
}

ChunkV::~ChunkV()
{
    for (const auto &Chunk : m_Chunks)
    {
        free((void *)Chunk);
    }
}

void ChunkV::CopyExternalToInternal()
{
    for (std::size_t i = 0; i < DataV.size(); ++i)
    {
        if (DataV[i].External)
        {
            size_t size = DataV[i].Size;
            // we can possibly append this entry to the tail if the tail entry
            // is internal
            bool AppendPossible = DataV.size() && !DataV.back().External;

            if (AppendPossible && (m_TailChunkPos + size > m_ChunkSize))
            {
                // No room in current chunk, close it out
                // realloc down to used size (helpful?) and set size in array
                m_Chunks.back() =
                    (char *)realloc(m_Chunks.back(), m_TailChunkPos);

                m_TailChunkPos = 0;
                m_TailChunk = NULL;
                AppendPossible = false;
            }
            if (AppendPossible)
            {
                // We can use current chunk, just append the data and modify the
                // DataV entry
                memcpy(m_TailChunk + m_TailChunkPos, DataV[i].Base, size);
                DataV[i].External = false;
                DataV[i].Base = m_TailChunk + m_TailChunkPos;
                m_TailChunkPos += size;
            }
            else
            {
                // We need a new chunk, get the larger of size or m_ChunkSize
                size_t NewSize = m_ChunkSize;
                if (size > m_ChunkSize)
                    NewSize = size;
                m_TailChunk = (char *)malloc(NewSize);
                m_Chunks.push_back(m_TailChunk);
                memcpy(m_TailChunk, DataV[i].Base, size);
                m_TailChunkPos = size;
                DataV[i] = {false, m_TailChunk, 0, size};
            }
        }
    }
}

size_t ChunkV::AddToVec(const size_t size, const void *buf, size_t align,
                        bool CopyReqd)
{
    if (size == 0)
    {
        return CurOffset;
    }

    AlignBuffer(align);
    size_t retOffset = CurOffset;

    if (!CopyReqd && !m_AlwaysCopy)
    {
        // just add buf to internal version of output vector
        VecEntry entry = {true, buf, 0, size};
        DataV.push_back(entry);
    }
    else
    {
        // we can possibly append this entry to the last if the last was
        // internal
        bool AppendPossible =
            DataV.size() && !DataV.back().External &&
            (m_TailChunk + m_TailChunkPos - DataV.back().Size ==
             DataV.back().Base);

        if (AppendPossible && (m_TailChunkPos + size > m_ChunkSize))
        {
            // No room in current chunk, close it out
            // realloc down to used size (helpful?) and set size in array
            m_Chunks.back() = (char *)realloc(m_Chunks.back(), m_TailChunkPos);

            m_TailChunkPos = 0;
            m_TailChunk = NULL;
            AppendPossible = false;
        }
        if (AppendPossible)
        {
            // We can use current chunk, just append the data;
            memcpy(m_TailChunk + m_TailChunkPos, buf, size);
            DataV.back().Size += size;
            m_TailChunkPos += size;
        }
        else
        {
            // We need a new chunk, get the larger of size or m_ChunkSize
            size_t NewSize = m_ChunkSize;
            if (size > m_ChunkSize)
                NewSize = size;
            m_TailChunk = (char *)malloc(NewSize);
            m_Chunks.push_back(m_TailChunk);
            memcpy(m_TailChunk, buf, size);
            m_TailChunkPos = size;
            VecEntry entry = {false, m_TailChunk, 0, size};
            DataV.push_back(entry);
        }
    }
    CurOffset = retOffset + size;
    return retOffset;
}

BufferV::BufferPos ChunkV::Allocate(const size_t size, size_t align)
{
    if (size == 0)
    {
        return BufferPos(-1, 0, CurOffset);
    }

    AlignBuffer(align);

    // we can possibly append this entry to the last if the last was
    // internal
    bool AppendPossible =
        DataV.size() && !DataV.back().External &&
        (m_TailChunk + m_TailChunkPos - DataV.back().Size == DataV.back().Base);

    if (AppendPossible && (m_TailChunkPos + size > m_ChunkSize))
    {
        // No room in current chunk, close it out
        // realloc down to used size (helpful?) and set size in array
        m_Chunks.back() = (char *)realloc(m_Chunks.back(), m_TailChunkPos);

        m_TailChunkPos = 0;
        m_TailChunk = NULL;
        AppendPossible = false;
    }

    size_t bufferPos = 0;
    if (AppendPossible)
    {
        // We can use current chunk, just append the data;
        bufferPos = m_TailChunkPos;
        DataV.back().Size += size;
        m_TailChunkPos += size;
    }
    else
    {
        // We need a new chunk, get the larger of size or m_ChunkSize
        size_t NewSize = m_ChunkSize;
        if (size > m_ChunkSize)
            NewSize = size;
        m_TailChunk = (char *)malloc(NewSize);
        m_Chunks.push_back(m_TailChunk);
        bufferPos = 0;
        m_TailChunkPos = size;
        VecEntry entry = {false, m_TailChunk, 0, size};
        DataV.push_back(entry);
    }

    BufferPos bp(static_cast<int>(DataV.size() - 1), bufferPos, CurOffset);
    // valid ptr anytime <-- DataV[idx] + bufferPos;

    CurOffset += size;

    return bp;
}

void *ChunkV::GetPtr(int bufferIdx, size_t posInBuffer)
{
    if (bufferIdx == -1)
    {
        return nullptr;
    }
    else if (static_cast<size_t>(bufferIdx) > DataV.size() ||
             DataV[bufferIdx].External)
    {
        throw std::invalid_argument(
            "ChunkV::GetPtr(" + std::to_string(bufferIdx) + ", " +
            std::to_string(posInBuffer) +
            ") refers to a non-existing or deferred memory chunk.");
    }
    else
    {
        return (void *)((char *)DataV[bufferIdx].Base + posInBuffer);
    }
}

std::vector<core::iovec> ChunkV::DataVec() noexcept
{
    std::vector<core::iovec> iov(DataV.size());
    for (std::size_t i = 0; i < DataV.size(); ++i)
    {
        // For ChunkV, all entries in DataV are actual iov entries.
        iov[i].iov_base = DataV[i].Base;
        iov[i].iov_len = DataV[i].Size;
    }
    return iov;
}

} // end namespace format
} // end namespace adios2
