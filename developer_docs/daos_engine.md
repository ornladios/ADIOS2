.. SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
..
.. SPDX-License-Identifier: Apache-2.0

# DAOS Engine: design notes

The DAOS engine writes ADIOS2 data through the DAOS object API
(`daos_array_write`, `daos_kv_put`) instead of through a POSIX file.
This document records the design decisions; the public API and end-user
configuration are described in `docs/user_guide/source/engines/daos.rst`.

## Goals

The engine targets long-running large-scale HPC workloads on sites
with a DAOS storage tier, accessed through the DAOS object API
rather than through a POSIX filesystem layered on top of DAOS.
The two main design choices follow from this:

  * Eliminate the BP5 aggregator step.  At per-node DAOS bandwidths
    of tens of GB/s, a single aggregator-per-node becomes the
    bottleneck.  The DAOS engine has every rank write its own data
    independently to a per-rank DAOS array.
  * Overlap data writes with subsequent Puts.  Async submits issued
    during the buffer's `AddToVec` run concurrently with the user's
    next `Put`.

## Data path: per-rank DAOS array, async during AddToVec

The fundamental data abstraction is one DAOS array per writer rank,
allocated in `OpenDataArray()`.  Each rank's array OID is allocated
server-side via `daos_cont_alloc_oids` (no MPI coordination needed)
and written into a small text index at `<dataset>/data_oids.txt` so
the reader can locate them.  A per-rank cursor `m_DataArrayCursor`
tracks where the next step's data will land in the array; it
advances in `WriteData` by `DataBuffer->Size()` per step.

Data flows through a `DaosChunkV` buffer, a subclass of `ChunkV`.
On each `AddToVec` from `BP5Serializer::Marshal`:

  1. The parent `ChunkV::AddToVec` does the memcpy or external-iov
     record, as usual.  Timed under `atvMemcpy`.
  2. A non-blocking `daos_eq_poll(timeout=0)` reaps any completed
     submits, freeing their `PendingSubmit` records.  Bounds the
     in-flight queue without blocking the user.  Timed as `atvReap`.
  3. If the byte threshold has been crossed (bytes accumulated since
     the last submit ≥ `m_ChunkSize`), one async `daos_array_write`
     covering everything since the last submit is fired with a
     multi-iov SGL: a single call with many iovs over a contiguous
     range.  Timed as `atvSubmit`, with sub-timers `subSetup`,
     `subEventInit`, `subWriteCall`.

At `EndStep`, `WriteData` calls `DaosChunkV::Drain()` (blocking poll
until the in-flight queue is empty) and then writes any unflushed
tail (still-filling chunk + any externs) synchronously with one
`daos_array_write` directly from the engine.

### Why `m_NoShrink` matters

`ChunkV` normally calls `realloc` on a chunk when it's "closed out"
(a new chunk is started past it) to release trailing waste.  That
realloc can move the pointer.  For DAOS we have async writes
in flight against those addresses; if the pointer moves while the
write is in flight, the I/O reads moved memory.

`DaosChunkV` sets the protected `m_NoShrink = true` flag in its
constructor, which gates the close-out shrink in
`ChunkV::AddToVec`/`Allocate`.  Trade-off: each finalized chunk may
have up to `(m_ChunkSize - actually_used)` bytes of trailing waste.

### Why chunks are recycled across steps

The engine's `m_DataBuf` is a single `DaosChunkV` allocated lazily
on the first `BeginStep` and `Reset()` between steps.  Resetting
rewinds `m_NextActiveChunk` to 0; subsequent allocations claim the
existing chunks in place rather than allocating fresh ones.  Two
benefits:

  * Page-fault elimination.  Fresh aligned_alloc requires
    first-touch zeroing on every step.  At 32 ranks × 800 MB/step
    this was ~30% of per-step wall.
  * Libfabric MR cache hot.  Stable buffer addresses across steps
    means the network's memory-region cache stays valid; otherwise
    each step re-pins fresh pages.

Chunk recycling lives in `ChunkV` (the parent), gated by a cursor
(`m_NextActiveChunk`) that subclasses pick up automatically.

### The deferred-extern path

If `Put(..., adios2::Mode::Deferred)` and the user's payload is
above `MinDeferredSize`, BP5Serializer records a `VecEntry` with
`External=true` pointing at the user's memory rather than copying.
DAOS's libfabric registers the user pointer at submit time.  Per-MR
cost dominates for small payloads, so `MinDeferredSize` defaults
exclude small Puts from the deferred path.

## Metadata path: split between DAOS and POSIX files

The engine writes metadata in two places:

| metadata kind                            | location                                              |
| ---------------------------------------- | ----------------------------------------------------- |
| Per-rank metadata blob                   | DAOS: `daos_array_write` to a global metadata array, OR `daos_kv_put` keyed by `step%zu-rank%d` |
| Per-step metadata-size index             | DAOS-KV: `mdsize_oh` keyed by `step%zu` (rank 0)      |
| Attribute blocks (rank-0 only, per step) | POSIX `md.0` via `m_FileMetadataManager` (rank 0)     |
| Meta-meta / FFS formats                  | POSIX `mmd.0` via `m_FileMetaMetadataManager` (rank 0)|
| Step index records                       | POSIX `md.idx` via `m_FileMetadataIndexManager` (rank 0) |
| Per-rank data-array OID index            | POSIX `data_oids.txt` (rank 0, written once at open)  |
| Metadata-side OIDs                       | POSIX `m_OIDFileName` (rank 0, written once at open)  |

The bulk of metadata (the per-rank `MetaEncodeBuffer` from
`BP5Serializer::CloseTimestep`) goes to DAOS, because it scales
with `NumWriters × NumVars × NumSteps`.  The smaller rank-0-only
index and attribute files use POSIX, where the path resolves to
whatever filesystem is mounted there.  On Aurora these POSIX
files typically land in a dfuse-mounted DAOS POSIX container
co-located with the data.

### The selective-aggregation gate

Most steps in stable HPC workloads have no new MetaMeta or new
Attributes — only the per-step variable metadata changes.  A cheap
1-byte `Allreduce(Op::Max)` of `localHasContent` decides whether to
run the BP5-style selective aggregation collective at all.  The
`anyHasContent==false` path at steady state:

  * Skips `BP5AggregateInformation` (the cross-rank collective).
  * Still gathers `m_StartDataPos` (one uint64 per rank to rank 0)
    so `WriteMetadataFileIndex` emits correct per-rank offsets.
  * Still writes a step record to `md.idx` (the reader's attribute
    path indexes by `m_MetadataIndexTable[Step][4]`, so every step
    needs an entry).

Aurora MPICH's non-blocking collectives don't auto-progress; an
`Iallreduce + Wait` version measured the same wall as `Allreduce`.
We keep it blocking.  Cost is ~80-100 ms/step at 32 ranks, the
floor for a single MPI collective at this scale on this network.

### The `m_StartDataPos` gather every step

`m_StartDataPos` is the per-rank array cursor at the start of the
step (= `m_DataArrayCursor` before the step's data is written).
The reader's `ReadData` uses this — via the per-writer block in
`md.idx` — to compute the byte offset within the writer's array.

The gather runs every step (separate `ES_GatherPos` timer), not
just on `anyHasContent` steps.  Without it, `m_WriterDataPos`
holds step-0 offsets forever and reads of step N return step 0's
data.  Cost is ~20 ms/step at 32 ranks.

Possible future improvement: encode the per-step start offset in
the per-rank metadata block (which the reader already parses) and
drop the gather.  Not done in the initial implementation.

### The OnetimeMarshalAttribute hand-off

Attributes have two BP5Serializer code paths:

  * `OnetimeMarshalAttribute` — registers an attribute once into a
    persistent FFS-format buffer; subsequent steps don't re-emit.
    Used at `BeginStep` of step 0 when `UseOneTimeAttributes` is
    true, and from `NotifyEngineAttribute` for attributes added
    mid-stream.
  * Regular `MarshalAttribute` — called from `MarshalAttributes()`
    at the top of `EndStep`, gated by `m_MarshalAttributesNecessary`.

When `OnetimeMarshalAttribute` is used at `BeginStep`,
`m_MarshalAttributesNecessary = false` must be set so `EndStep`
doesn't double-register through the regular path.  Symptom of the
bug: attribute block has a different FFS format ID than BP5's, so
the reader's deserializer rejects it.  The flag clear must travel
with any future refactor of the attribute lifecycle.

## DAOS lifecycle: init/fini, handle release

`daos_init()` runs in `InitDAOS()`, on every rank.

`daos_fini()` is **not** called at close.  We tried adding it; any
subsequent `daos_init()` in the same process produced
`d_hhash_link_lookup` errors and an `obj_ec_update_iod_size`
assertion during the read.  The failure is in libdaos's reinit
path, so refcounting on our side wouldn't help.  libdaos stays
initialized for process lifetime; the OS reaps at exit.

Handle close (pool, container, array, KV, event queue) IS called
at `DoClose` and is safe.  In long-running multi-engine processes
this is what prevents handle leaks.

## Profiling

The engine uses `JSONProfiler` (BP5-style).  Phase timers
(`OpenSetup`, `WD_Drain`, `WD_Tail`, `MD_Daos`, `MD_Posix`,
`PutCommon`, `PutCommon_Marshal`, `BS`, `ES`, `ES_CloseTS`,
`ES_SelAgg`, `ES_Gate`, `ES_GatherPos`, `BetweenSteps`) are
registered in `RegisterProfilerTimers()` and accumulated through
`ProfilerGuard` RAII brackets.  The DaosChunkV-internal sub-timers
(`atvMemcpy`, `atvReap`, `atvSubmit`, `subSetup`, `subEventInit`,
`subWriteCall`) live in the same profiler, registered with the
same names.

`FlushProfiler` writes the standard `<dataset>/profiling.json`
with every rank's per-timer JSON.

`BP5Serializer` exposes `m_BufferAppendSecs` / `m_BufferAppendCalls`
/ `m_GetMinMaxSecs` accumulators that live outside the profiler,
and `DaosChunkV` exposes `FreshAllocCount()`.  These four values
are mirrored into registered profiler timers (`BP5_BufferAppend`,
`BP5_GetMinMax`, `FreshChunkAllocs`) at `DoClose` by
`SyncExternalCountersToProfiler`, so they appear in
`profiling.json` alongside the rest.  Cross-rank aggregation
(max/min/avg) is left to post-processing.

## Span semantics: a known limitation

ADIOS Spans (`Put(var, span, init, value)`) work for the common
"engine fills the span before any subsequent Put" case (operators,
writer-side memory selection, derived variables).  They do NOT
correctly handle the rarer "user defers fill until just before
EndStep" case: chunk-fill async submit can fire from a subsequent
`AddToVec` while the user-facing span is still unfilled, writing
zero/stale bytes to DAOS.  Tracked as a known limitation; the fix
will require either tracking allocated-but-unfilled span ranges
(so chunk-fill submit refuses to cross them) or suppressing
mid-step submit while any span is outstanding.  Until that lands,
the engine treats Span Puts as unsupported (see user doc).

## Engine parameters

  * `BufferChunkSize` — `DaosChunkV` chunk size AND the DAOS array's
    internal chunk_size.  Default `DefaultBufferChunkSize` (128 MiB).
    A useful starting point for overlap is roughly
    `(per-step data per rank) / 4`.
  * `MinDeferredSize` — Puts smaller than this are routed sync.
    Routes around the per-iov MR registration cost dominating
    for small payloads.  Default `DefaultMinDeferredSize` (4 MiB).
  * `StatsLevel` — 0 disables `GetMinMax` on the data path.
  * `UseOneTimeAttributes` — if true (the default), attributes are
    marshalled once at BeginStep step 0 rather than re-marshalled
    every step.

## Files

  * `source/adios2/engine/daos/DaosWriter.{h,cpp,tcc}` — writer.
  * `source/adios2/engine/daos/DaosReader.{h,cpp,tcc}` — reader.
  * `source/adios2/toolkit/format/buffer/chunk/DaosChunkV.{h,cpp}` —
    buffer subclass of `ChunkV` that submits async daos_array_write
    during AddToVec.
  * `source/adios2/toolkit/format/buffer/chunk/ChunkV.{h,cpp}` —
    parent buffer; gained `Reset()`, `m_NoShrink`, and chunk-
    recycling machinery used by `DaosChunkV`.
  * `source/adios2/toolkit/format/bp5/BP5Serializer.{h,cpp}` —
    sub-Marshal timing accumulators (`m_BufferAppendSecs`,
    `m_BufferAppendCalls`, `m_GetMinMaxSecs`).
