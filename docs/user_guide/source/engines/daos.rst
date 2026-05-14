.. SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
..
.. SPDX-License-Identifier: Apache-2.0

****
DAOS
****

The DAOS engine writes and reads ADIOS2 datasets through the
`DAOS <https://docs.daos.io>`_ object-store API directly, instead of
through a POSIX file.  On HPC sites with a DAOS storage tier this
bypasses the POSIX layer that BP5 uses against a dfuse-mounted DAOS
container.

Each writer rank writes its data to its own DAOS array.  During the
step the engine issues async ``daos_array_write`` submits as the
internal buffer accumulates each chunk's worth of data; subsequent
``Put`` calls overlap the in-flight DAOS transfer.  At ``EndStep``
the engine drains the in-flight queue and flushes any remaining tail
synchronously.  Each rank writes its own data independently; the
data path performs no cross-rank aggregation.

Metadata is split.  The bulk of metadata (per-rank, per-step) goes
to DAOS objects.  The smaller rank-0 metadata files (the ``md.idx``
step index, ``md.0`` attribute file, and ``mmd.0`` FFS format file)
are written as POSIX files in the dataset directory.

Build
=====

The engine requires DAOS development headers and libraries
(``libdaos``, ``libdaos_common``).  Configure ADIOS2 with
``-DADIOS2_USE_DAOS=ON``; CMake will probe for DAOS via ``pkg-config``.

Runtime requirements
====================

The DAOS engine uses the DAOS object API directly.  It does **not**
require the dfuse FUSE filesystem or any LD_PRELOAD interception
library; those are needed only when accessing DAOS as a POSIX
filesystem.

The engine **does** require:

* A DAOS pool and container that the user has access to.
* The DAOS agent reachable through ``DAOS_AGENT_DRPC_DIR``.
* Two environment variables on every rank:

  * ``DAOS_POOL`` — pool label or UUID.
  * ``DAOS_CONT`` — container label or UUID.

The pool and container must already exist before the writer opens the
engine; the engine does not create them.

.. note::

   HPC sites that ship a DAOS client module (for example
   ``daos/base`` on ALCF Aurora) typically use it to export
   libfabric and Mercury tuning environment variables — on
   Slingshot fabrics these include ``FI_CXI_DEFAULT_CQ_SIZE``,
   ``FI_CXI_RX_MATCH_MODE``, ``FI_MR_CACHE_MAX_COUNT``, and
   ``NA_OFI_CXI_PROTO_RNR``.  These are needed for the DAOS client
   to sustain the network load of many concurrent ranks.

   ``module load`` sets these in the launching shell, but they are
   **not** automatically propagated to ranks by every MPI launcher.
   Propagate them explicitly:

   * MPICH / PALS ``mpiexec``: ``-genvall``, or specific variables
     via ``--env NAME=VALUE``.
   * SLURM ``srun``: ``--export=ALL`` (or list specific variables).
   * OpenMPI ``mpirun``: typically propagates by default; ``-x NAME``
     to be explicit.

   Without propagation the ranks run with libfabric defaults that
   are too small for high-rank-count concurrent DAOS reads.  The
   visible symptom is ``DER_HG`` (Mercury transport error) followed
   by ``DER_TIMEDOUT`` after ~120 s during multi-node reads; the
   workload completes but read throughput drops by one to two
   orders of magnitude.

   On Aurora specifically, propagating the full module env to BP5
   ranks running on Lustre has been observed to break MPICH/Lustre
   I/O.  When a single submit script runs both DAOS and BP5 phases,
   apply ``-genvall`` only to the DAOS-phase ``mpiexec`` line.

Usage
=====

.. code-block:: c++

   io.SetEngine("daos");
   adios2::Engine daosFile = io.Open("name", adios2::Mode::Write);

The dataset is a directory rooted at ``name``:

.. code-block:: bash

   name/                       # dataset directory
       md.0                    # attribute blob (rank 0)
       md.idx                  # step index (rank 0)
       mmd.0                   # FFS format definitions (rank 0)
       data_oids.txt           # per-rank DAOS-array OID index
       oid.txt                 # metadata-side DAOS object IDs

The bulk per-rank data lives **in DAOS objects** rooted in the
container, located by ``data_oids.txt``.

The dataset directory itself is on a POSIX-accessible filesystem.
On systems where the application's working filesystem already lives
on a DAOS POSIX container (via dfuse), the directory will land
there; otherwise it lands on whatever filesystem the path resolves
to.  The choice of filesystem for the directory is independent of
the bulk data path, which always uses DAOS objects.

Reader-side requires the same ``DAOS_POOL`` and ``DAOS_CONT``
environment variables and access to the dataset directory (so the
reader can read ``data_oids.txt`` and the metadata files).

Parameters
==========

This engine accepts the following optional parameters via
``IO::SetParameter`` (or its language equivalents):

1. **BufferChunkSize**: Size in bytes of each internal data chunk.
   Async ``daos_array_write`` is triggered whenever the bytes
   accumulated since the last submit cross this threshold; larger
   values produce fewer, larger DAOS writes.  This value is also
   used as the internal ``chunk_size`` of the DAOS array on
   creation.  Default is 128 MiB.  When per-step data per rank is
   small, setting this value to roughly one quarter of that size
   keeps multiple submits in flight per step.

2. **MinDeferredSize**: Puts with payload smaller than this are
   routed through a synchronous (memcpy-into-buffer) path rather
   than the deferred-extern (zero-copy from user pointer) path.
   Per-iov memory-region registration dominates DAOS submit time
   for small payloads; routing them through the buffer batches them
   into a single iov.  Default is 4 MiB.

3. **StatsLevel**: 0 disables the per-Put GetMinMax computation,
   which removes that work from the data path.  Default is 1.

4. **UseOneTimeAttributes**: If true, attributes are marshalled once
   at the first ``BeginStep`` instead of re-marshalled every step.
   Lowers per-step metadata cost when attributes are static.

The metadata-layout choice is exposed through the environment
variable ``DAOS_METADATA_LAYOUT`` rather than an IO parameter.  Its
default (``array-1mb-aligned``) is appropriate for typical use; the
other values (``array``, ``kv``) exist for benchmarking alternative
DAOS metadata storage strategies and are not normally needed.

Performance notes
=================

* Open costs (pool connect, container open, OID allocation) are
  paid once per engine and amortize across all steps.  Async
  submits issued during ``Put`` overlap with subsequent ``Put``
  calls in the same step.
* Because every rank writes its own data, scaling is per-rank
  rather than per-aggregator-node.  Per-node bandwidth is set by
  the rank-to-DAOS-server fabric rather than by a single
  aggregator process.

Limitations
===========

* **Span Puts are not supported.**  ``Put(var, span, init, value)``
  cannot meet ADIOS semantics in this engine: async submits during
  subsequent ``Put`` calls may flush the span's bytes to DAOS
  before the user has filled it.
* Append mode is not supported (``Mode::Append`` is rejected at
  open).
* Both ``Mode::Read`` (step-by-step) and ``Mode::ReadRandomAccess``
  are supported on the read side.
