/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DaosChunkV.h"

#include "adios2/helper/adiosLog.h"

#include <algorithm>
#include <cstdlib> // getenv, strtoull

namespace adios2
{
namespace format
{

DaosChunkV::DaosChunkV(const std::string type, bool AlwaysCopy, size_t MemAlign,
                       size_t MemBlockSize, size_t ChunkSize, daos_handle_t arrayHandle,
                       daos_handle_t eventQueue, daos_size_t baseArrayOffset,
                       profiling::JSONProfiler &profiler)
: ChunkV(type, AlwaysCopy, MemAlign, MemBlockSize, ChunkSize), m_ArrayHandle(arrayHandle),
  m_EventQueue(eventQueue), m_BaseArrayOffset(baseArrayOffset), m_Profiler(profiler)
{
    // Required for pointer stability of finalized chunks while async
    // writes referencing those bytes are in flight.
    m_NoShrink = true;
}

DaosChunkV::~DaosChunkV()
{
    // Defensive: in normal flow Drain() has been called from the engine
    // and Reset(), but if the buffer is destroyed without that we still
    // need to wait for any outstanding events before the engine tears
    // down the shared EQ at engine close.
    try
    {
        Drain();
    }
    catch (...)
    {
        // Swallow: destructor must not throw.  Errors here would also
        // have been raised earlier, ideally.
    }
}

size_t DaosChunkV::AddToVec(size_t size, const void *buf, size_t align, bool CopyReqd,
                            MemorySpace MemSpace)
{
    size_t off;
    {
        profiling::ProfilerGuard g(m_Profiler, "atvMemcpy");
        off = ChunkV::AddToVec(size, buf, align, CopyReqd, MemSpace);
    }

    {
        // Cheap, non-blocking reap so m_Pending doesn't grow unbounded.
        profiling::ProfilerGuard g(m_Profiler, "atvReap");
        ReapCompleted();
    }

    if (size == 0)
        return off;

    // Unified byte-threshold trigger.  Whenever the bytes accumulated
    // since the last submission cross m_ChunkSize, submit everything
    // since then as ONE daos_array_write with a multi-iov SGL.  Iovs
    // can be a mix of internal-chunk pointers and external (user
    // memory) pointers; both stay valid for the rest of the step
    // (chunks via m_NoShrink, externals via the deferred contract).
    //
    // This is mode B from tier-1: one call, N iovs, contiguous range.
    // Replaces an earlier design that fired immediately per deferred-
    // extern entry (which collapsed to mode A and ate all the savings
    // from skipping the memcpy).
    const size_t pending = CurOffset - m_LastSubmittedOffset;
    if (pending >= m_ChunkSize)
    {
        profiling::ProfilerGuard g(m_Profiler, "atvSubmit");
        SubmitRange(m_LastSubmittedDataVIdx, DataV.size() - 1);
        m_LastSubmittedDataVIdx = DataV.size();
        m_LastSubmittedOffset = CurOffset;
    }
    return off;
}

BufferV::BufferPos DaosChunkV::Allocate(size_t size, size_t align)
{
    // Span allocation: the user has not yet filled the bytes, so we
    // cannot submit now.  ChunkV::Allocate may itself finalize a chunk
    // if alignment forces a roll-over; we conservatively do NOT detect
    // that here (the user may still write into the just-allocated span,
    // which lives in the new tail chunk).  The next AddToVec that
    // finalizes the tail will pick up everything; otherwise the
    // EndStep tail path handles it.
    return ChunkV::Allocate(size, align);
}

std::vector<core::iovec> DaosChunkV::DataVec() noexcept { return ChunkV::DataVec(); }

std::vector<core::iovec> DaosChunkV::UnflushedTail() noexcept
{
    std::vector<core::iovec> out;
    out.reserve(DataV.size() - m_LastSubmittedDataVIdx);
    for (size_t i = m_LastSubmittedDataVIdx; i < DataV.size(); ++i)
    {
        const VecEntry &v = DataV[i];
        const void *base = v.External ? v.Base : (const char *)v.Base + v.Offset;
        out.push_back(core::iovec{const_cast<void *>(base), v.Size});
    }
    return out;
}

void DaosChunkV::SubmitRange(size_t firstIdx, size_t lastIdx)
{
    if (firstIdx > lastIdx || lastIdx >= DataV.size())
        return;

    // Assemble the contiguous iov set for this range.
    std::vector<d_iov_t> allIovs;
    daos_size_t total = 0;
    {
        profiling::ProfilerGuard g(m_Profiler, "subSetup");
        allIovs.reserve(lastIdx - firstIdx + 1);
        for (size_t i = firstIdx; i <= lastIdx; ++i)
        {
            const VecEntry &v = DataV[i];
            if (v.Size == 0)
                continue;
            const void *base = v.External ? v.Base : (const char *)v.Base + v.Offset;
            d_iov_t iov;
            iov.iov_buf = const_cast<void *>(base);
            iov.iov_buf_len = v.Size;
            iov.iov_len = v.Size;
            allIovs.push_back(iov);
            total += v.Size;
        }
    }
    if (total == 0)
        return;

    // Chunk large writes to bound Mercury bulk registration.  A single
    // daos_array_write of the full pending range (up to BufferChunkSize,
    // 128 MiB) creates one large bulk transfer; issued concurrently across
    // ranks against a loaded/degraded pool this stresses Mercury's NA
    // memory descriptors -- the write-side mirror of the read DER_NOMEM
    // crash (see DaosReader::ReadData).  Splitting into bounded sub-writes,
    // each its own async event, keeps every bulk transfer small (the same
    // strategy IOR-DFS uses via --dfs.chunk_size) and lets earlier events
    // reap/free before later ones register.  DAOS_WRITE_CHUNK_BYTES=0
    // restores the single-write mode-B behavior for A/B testing.
    //
    // Default 16 MiB: a same-pool A/B sweep (job 8659453, 32r x 256 MiB x 4)
    // showed the unchunked path at 5.31 GiB/s with 128 DER_HG mercury errors,
    // vs 10.7-10.8 GiB/s and ZERO errors at 4-16 MiB (2x, and the retries the
    // errors forced are gone).  1 MiB was past the sweet spot -- ~8K in-flight
    // events/step reintroduced pressure and it did not complete.  16 MiB sits
    // at the top of the plateau and matches the read path's default.
    static const size_t writeChunk = []() -> size_t {
        if (const char *e = std::getenv("DAOS_WRITE_CHUNK_BYTES"))
            return static_cast<size_t>(std::strtoull(e, nullptr, 10));
        return size_t{16} * 1024 * 1024; // 16 MiB default
    }();
    const size_t step = (writeChunk == 0) ? static_cast<size_t>(total) : writeChunk;

    const daos_size_t startOff = m_BaseArrayOffset + m_SubmittedHigh;
    daos_size_t rel = 0; // bytes of this range already assigned to a sub-write
    size_t i = 0;        // index into allIovs
    size_t inIovOff = 0; // bytes of allIovs[i] already consumed
    while (i < allIovs.size())
    {
        std::unique_ptr<PendingSubmit> p;
        {
            profiling::ProfilerGuard g(m_Profiler, "subSetup");
            p = std::make_unique<PendingSubmit>();
            daos_size_t batchBytes = 0;
            while (i < allIovs.size() && batchBytes < step)
            {
                const size_t avail = allIovs[i].iov_len - inIovOff;
                const size_t take = std::min(avail, step - static_cast<size_t>(batchBytes));
                d_iov_t v;
                v.iov_buf = static_cast<char *>(allIovs[i].iov_buf) + inIovOff;
                v.iov_buf_len = take;
                v.iov_len = take;
                p->iovs.push_back(v);
                batchBytes += take;
                inIovOff += take;
                if (inIovOff == allIovs[i].iov_len)
                {
                    ++i;
                    inIovOff = 0;
                }
            }
            p->range.rg_idx = startOff + rel;
            p->range.rg_len = batchBytes;
            p->iod.arr_nr = 1;
            p->iod.arr_rgs = &p->range;
            p->sgl.sg_nr = static_cast<uint32_t>(p->iovs.size());
            p->sgl.sg_nr_out = 0;
            p->sgl.sg_iovs = p->iovs.data();
            rel += batchBytes;
        }

        int rc;
        {
            profiling::ProfilerGuard g(m_Profiler, "subEventInit");
            rc = daos_event_init(&p->event, m_EventQueue, /*parent=*/nullptr);
        }
        if (rc)
        {
            helper::Throw<std::runtime_error>("Toolkit", "format::DaosChunkV", "SubmitRange",
                                              "daos_event_init failed rc=" + std::to_string(rc));
        }

        {
            profiling::ProfilerGuard g(m_Profiler, "subWriteCall");
            rc = daos_array_write(m_ArrayHandle, DAOS_TX_NONE, &p->iod, &p->sgl, &p->event);
        }
        if (rc)
        {
            // The event was initialized but the call failed before queuing;
            // tear down the event so we don't leak.
            daos_event_fini(&p->event);
            helper::Throw<std::runtime_error>("Toolkit", "format::DaosChunkV", "SubmitRange",
                                              "daos_array_write failed rc=" + std::to_string(rc));
        }

        m_Pending.push_back(std::move(p));
    }
    m_SubmittedHigh += total;
}

void DaosChunkV::ReapCompleted()
{
    if (m_Pending.empty())
        return;
    daos_event_t *done[16];
    for (;;)
    {
        // Non-blocking: timeout=0 means poll-and-return-immediately.
        int nd = daos_eq_poll(m_EventQueue, /*wait_running=*/0,
                              /*timeout_us=*/0, 16, done);
        if (nd <= 0)
            break;
        for (int i = 0; i < nd; ++i)
        {
            // ev_error is the per-call rc.  Surface non-zero as a throw
            // so we don't silently lose data.
            int ev_err = done[i]->ev_error;
            auto it = std::find_if(
                m_Pending.begin(), m_Pending.end(),
                [&](const std::unique_ptr<PendingSubmit> &p) { return &p->event == done[i]; });
            if (it != m_Pending.end())
            {
                daos_event_fini(&(*it)->event);
                m_Pending.erase(it);
            }
            if (ev_err)
            {
                helper::Throw<std::runtime_error>("Toolkit", "format::DaosChunkV", "ReapCompleted",
                                                  "completed event reports error rc=" +
                                                      std::to_string(ev_err));
            }
        }
    }
}

void DaosChunkV::Drain()
{
    daos_event_t *done[16];
    while (!m_Pending.empty())
    {
        int nd = daos_eq_poll(m_EventQueue, /*wait_running=*/1,
                              /*timeout_us=*/-1 /* DAOS_EQ_WAIT */, 16, done);
        if (nd <= 0)
            continue;
        for (int i = 0; i < nd; ++i)
        {
            int ev_err = done[i]->ev_error;
            auto it = std::find_if(
                m_Pending.begin(), m_Pending.end(),
                [&](const std::unique_ptr<PendingSubmit> &p) { return &p->event == done[i]; });
            if (it != m_Pending.end())
            {
                daos_event_fini(&(*it)->event);
                m_Pending.erase(it);
            }
            if (ev_err)
            {
                helper::Throw<std::runtime_error>("Toolkit", "format::DaosChunkV", "Drain",
                                                  "completed event reports error rc=" +
                                                      std::to_string(ev_err));
            }
        }
    }
}

void DaosChunkV::Reset()
{
    Drain();
    ChunkV::Reset();
    m_SubmittedHigh = 0;
    m_LastSubmittedDataVIdx = 0;
    m_LastSubmittedOffset = 0;
}

} // namespace format
} // namespace adios2
