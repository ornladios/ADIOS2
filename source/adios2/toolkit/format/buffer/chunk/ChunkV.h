/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_MALLOC_CHUNKV_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_MALLOC_CHUNKV_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/CoreTypes.h"

#include "adios2/toolkit/format/buffer/BufferV.h"

namespace adios2
{
namespace format
{

class ChunkV : public BufferV
{
public:
    uint64_t Size() noexcept;

    const size_t m_ChunkSize;

    ChunkV(const std::string type, const bool AlwaysCopy = false, const size_t MemAlign = 1,
           const size_t MemBlockSize = 1, const size_t ChunkSize = DefaultBufferChunkSize);
    virtual ~ChunkV();

    std::vector<core::iovec> DataVec() noexcept override;

    size_t AddToVec(const size_t size, const void *buf, size_t align, bool CopyReqd,
                    MemorySpace MemSpace = MemorySpace::Host) override;

    BufferPos Allocate(const size_t size, size_t align) override;
    void DownsizeLastAlloc(const size_t oldSize, const size_t newSize) override;

    void *GetPtr(int bufferIdx, size_t posInBuffer) override;
    void *GetPtr(size_t OverallPosInBuffer) override;

    void CopyDataToBuffer(const size_t size, const void *buf, size_t pos, MemorySpace MemSpace);

    /** Reset in-step state so the existing chunks can be reused for the
     *  next step.  Chunks themselves remain allocated and keep their
     *  addresses stable — that's the whole point of recycling: downstream
     *  MR caches (libfabric/CXI) stay hot, and we avoid the per-step
     *  page-fault cost of fresh allocations. */
    void Reset() override;

    /** Number of fresh chunk allocations over the buffer's lifetime.
     *  Grows during step 0 as the working set is established; should
     *  stay flat afterward if recycling is working.  A growing count
     *  past step 0 means a step needed more or larger chunks than the
     *  pool had — typically indicates a workload-shape change. */
    size_t FreshAllocCount() const { return m_FreshAllocCount; }

protected:
    /** When true, finalizing a chunk does NOT realloc-shrink it.  Pointers
     *  stay stable for the chunk's lifetime, which is required if the
     *  chunk's data may be referenced by an in-flight async I/O (e.g.
     *  DaosChunkV streaming async daos_array_writes during AddToVec).
     *  Trade-off: each chunk may have up to (m_ChunkSize - actually_used)
     *  bytes of trailing waste. */
    bool m_NoShrink = false;

    struct Chunk
    {
        char *Ptr;          // aligned, do not free
        void *AllocatedPtr; // original ptr, free this
        size_t Size;
    };

    std::vector<Chunk> m_Chunks;
    size_t m_TailChunkPos = 0;
    Chunk *m_TailChunk = nullptr;

    /** Index of the next chunk to claim from m_Chunks on roll-over.
     *  When AddToVec / Allocate needs a fresh chunk (because the current
     *  one is full or there isn't one yet), it first tries m_Chunks at
     *  this index.  If still in range AND large enough, we recycle that
     *  chunk in-place (no aligned_alloc, no first-touch fault).  Else
     *  fall through to ChunkAlloc + push_back. */
    size_t m_NextActiveChunk = 0;

    /** Counts only fresh ChunkAlloc calls, not recycled-chunk reuses.
     *  Surfaces "is recycling actually working?". */
    size_t m_FreshAllocCount = 0;

    // allocator function, doing aligned allocation of memory
    // return true if (re)allocation is successful
    // on failure, VecEntry is unmodified
    size_t ChunkAlloc(Chunk &v, const size_t size);
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_MALLOC_MALLOCV_H_ */
