/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferV.cpp
 *
 */

#include "ChunkV.h"
#include "adios2/toolkit/format/buffer/BufferV.h"
#include <iostream>
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
        std::cout << "freeing chunk " << (void *)Chunk << std::endl;
        free((void *)Chunk);
    }
}

size_t ChunkV::AddToVec(const size_t size, const void *buf, int align,
                        bool CopyReqd)
{
    std::cout << "In add to vec.  size " << size << " buf " << (void *)buf
              << " align " << align << " Copy " << CopyReqd << std::endl;
    int badAlign = CurOffset % align;
    if (badAlign)
    {
        int addAlign = align - badAlign;
        char zero[16] = {0};
        AddToVec(addAlign, zero, 1, true);
    }
    size_t retOffset = CurOffset;

    if (size == 0)
        return CurOffset;

    std::cout << "In add to vec" << std::endl;
    if (!CopyReqd && !m_AlwaysCopy)
    {
        // just add buf to internal version of output vector
        VecEntry entry = {true, buf, 0, size};
        DataV.push_back(entry);
        std::cout << "In add to vec2" << std::endl;
    }
    else
    {
        // we can possibly append this entry to the last if the last was
        // internal
        std::cout << "In add to vec3" << std::endl;
        bool AppendPossible = DataV.size() && !DataV.back().External;

        if (AppendPossible && (m_TailChunkPos + size > m_ChunkSize))
        {
            // No room in current chunk, close it out
            // realloc down to used size (helpful?) and set size in array
            std::cout << "In add to vec5" << std::endl;
            m_Chunks.back() = (char *)realloc(m_Chunks.back(), m_TailChunkPos);

            m_TailChunkPos = 0;
            m_TailChunk = NULL;
            AppendPossible = false;
        }
        if (AppendPossible)
        {
            std::cout << "In add to vec4" << std::endl;
            // We can use current chunk, just append the data;
            memcpy(m_TailChunk + m_TailChunkPos, buf, size);
            DataV.back().Size += size;
            m_TailChunkPos += size;
        }
        else
        {
            // We need a new chunk, get the larger of size or m_ChunkSize
            std::cout << "In add to vec6 m_ChunkSize" << m_ChunkSize << std::endl;
            size_t NewSize = m_ChunkSize;
            std::cout << "In add to vec7" << std::endl;
            if (size > m_ChunkSize)
                NewSize = size;
            std::cout << "In add to vec8 new size" << NewSize << std::endl;
            m_TailChunk = (char *)malloc(NewSize);
            std::cout << "In add to vec9 size" << size << std::endl;
            memcpy(m_TailChunk, buf, size);
            m_TailChunkPos = size;
            VecEntry entry = {false, m_TailChunk, 0, size};
            std::cout << "In add to vec10 size" << m_TailChunkPos << std::endl;
            DataV.push_back(entry);
        }
    }
    CurOffset = retOffset + size;
    return retOffset;
}

ChunkV::BufferV_iovec ChunkV::DataVec() noexcept
{
    BufferV_iovec ret = new iovec[DataV.size() + 1];
    for (std::size_t i = 0; i < DataV.size(); ++i)
    {
        // For ChunkV, all entries in DataV are actual iov entries.
        ret[i].iov_base = DataV[i].Base;
        ret[i].iov_len = DataV[i].Size;
    }
    ret[DataV.size()] = {NULL, 0};
    return ret;
}

} // end namespace format
} // end namespace adios2
