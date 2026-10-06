/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_CHUNK_DAOSCHUNKV_H_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_CHUNK_DAOSCHUNKV_H_

#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

#include <daos.h>
#include <daos_array.h>

#include <memory>
#include <vector>

namespace adios2
{
namespace format
{

/**
 * ChunkV variant that streams to a per-rank DAOS array.
 *
 * Behavior overview:
 *   - When a chunk is finalized (a new chunk is allocated past it), the
 *     bytes that were just appended to the prior tail chunk are submitted
 *     as one async daos_array_write covering a contiguous range of the
 *     rank's array, with one iov per VecEntry in the SGL.
 *   - When a deferred-extern entry (CopyReqd=false) is added, the entry
 *     is submitted immediately as one async daos_array_write directly
 *     from the user's pointer.  The user's deferred contract guarantees
 *     pointer stability through EndStep.
 *   - At EndStep, the engine calls Drain() to wait for all outstanding
 *     events, then writes any remaining tail (the still-filling chunk
 *     plus any external entries that raced) synchronously via one
 *     daos_array_write directly on the per-rank array handle.
 *
 * Realloc-shrink is disabled (m_NoShrink=true in ChunkV) so finalized
 * chunks do not move while their data is in flight.  Trade-off: each
 * finalized chunk may have up to (m_ChunkSize - actually_used) trailing
 * bytes of waste; bounded and small relative to step data.
 *
 * The class holds a non-owning daos_handle_t for the array and a
 * non-owning reference to a daos_event_queue.  Long-lived DAOS handles
 * (pool/container/array/EQ) are owned by the engine.
 */
class DaosChunkV : public ChunkV
{
public:
    /**
     * @param type             diagnostic label, mirrors parent
     * @param AlwaysCopy       force-copy flag, mirrors parent
     * @param MemAlign         byte alignment for chunk pointers
     * @param MemBlockSize     parent buffer block size
     * @param ChunkSize        per-chunk size; tier-1 sweet spot is
     *                         roughly (per-step data) / 4
     * @param arrayHandle      open DAOS array handle, owned elsewhere
     * @param eventQueue       open DAOS event queue, owned elsewhere
     *                         (engine-lifetime).  daos_eq_create costs
     *                         ~300 ms per call, so creating one per step
     *                         was a major BeginStep bottleneck — we now
     *                         share one EQ across steps.
     * @param baseArrayOffset  byte offset within the array where this
     *                         step's data begins (the engine's running
     *                         per-rank cursor)
     */
    DaosChunkV(const std::string type, bool AlwaysCopy, size_t MemAlign, size_t MemBlockSize,
               size_t ChunkSize, daos_handle_t arrayHandle, daos_handle_t eventQueue,
               daos_size_t baseArrayOffset, profiling::JSONProfiler &profiler);

    ~DaosChunkV() override;

    // Override: appends as ChunkV does, then triggers async submission
    // when (a) a chunk was just finalized or (b) the just-added entry
    // is a deferred-extern.
    size_t AddToVec(size_t size, const void *buf, size_t align, bool CopyReqd,
                    MemorySpace MemSpace = MemorySpace::Host) override;

    // ChunkV::Allocate path used by Span Puts.  We do not submit on
    // Allocate (the user has not yet filled the bytes); the next AddToVec
    // that finalizes this chunk will pick it up, or the EndStep tail
    // path will.
    BufferPos Allocate(size_t size, size_t align) override;

    // Returns the full iov set across the step, identical to parent's
    // semantics.  Used by the engine for metadata accounting (which var
    // lives at which buffer offset).  NOT used for I/O routing.
    std::vector<core::iovec> DataVec() noexcept override;

    /** Iovs that have not yet been async-submitted.  After Drain(), the
     *  engine writes these synchronously with one daos_array_write at
     *  array offset (m_BaseArrayOffset + SubmittedHigh()). */
    std::vector<core::iovec> UnflushedTail() noexcept;

    /** Block until all in-flight async writes have completed.  Throws on
     *  any per-event error. */
    void Drain();

    /** Bytes already submitted to the array since the start of this
     *  step.  After Drain(), data in [m_BaseArrayOffset,
     *  m_BaseArrayOffset + SubmittedHigh()) is on stable storage. */
    daos_size_t SubmittedHigh() const { return m_SubmittedHigh; }

    void Reset() override;

    /** Update the base array offset for the next step.  The engine
     *  advances its per-rank cursor between steps; with chunk recycling
     *  the same DaosChunkV instance handles every step, so the engine
     *  must tell it where in the array to start submitting. */
    void SetBaseArrayOffset(daos_size_t off) { m_BaseArrayOffset = off; }

private:
    daos_handle_t m_ArrayHandle; // not owned
    daos_handle_t m_EventQueue;  // not owned (engine-lifetime)
    daos_size_t m_BaseArrayOffset;
    profiling::JSONProfiler &m_Profiler; // not owned (engine-lifetime)

    daos_size_t m_SubmittedHigh = 0; // bytes submitted, this step
    // Submit cursor: the first unsubmitted byte is m_SubmitInEntry bytes into
    // DataV[m_SubmitIdx].  Byte-granular because the tail entry is submitted while
    // its chunk is still open, and later copied Puts append to that same entry.
    size_t m_SubmitIdx = 0;
    size_t m_SubmitInEntry = 0;
    size_t m_LastSubmittedOffset =
        0; // byte offset (CurOffset) at last submission; trigger when pending >= chunk_size

    /** One in-flight (or completed-but-not-yet-reaped) submission. */
    struct PendingSubmit
    {
        daos_event_t event;
        daos_range_t range;
        daos_array_iod_t iod;
        d_sg_list_t sgl;
        std::vector<d_iov_t> iovs; // backing storage for sgl.sg_iovs
    };
    std::vector<std::unique_ptr<PendingSubmit>> m_Pending;

    /** Iovs for every byte at or after the submit cursor. */
    std::vector<d_iov_t> PendingIovs() const;

    /** Submit everything after the submit cursor as async writes starting at
     *  m_BaseArrayOffset + m_SubmittedHigh, then advance the cursor to the end
     *  of DataV and m_SubmittedHigh by the bytes submitted. */
    void SubmitPending();

    /** Reap completed events and free their PendingSubmits.  wait=false polls
     *  once without blocking; wait=true blocks until m_Pending is empty.
     *  Throws on the first per-event error, but only after finalizing every
     *  event the poll returned. */
    void Reap(bool wait, const char *caller);
};

} // namespace format
} // namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_CHUNK_DAOSCHUNKV_H_ */
