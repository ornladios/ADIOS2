########################
SST Performance Tuning
########################

This page explains how to get good streaming performance from the
:ref:`SST Sustainable Staging Transport` engine on HPC systems: which settings matter, how to
launch on the systems listed here, and what to check when a workflow runs slower than expected.
Rates measured on those systems are collected in `Reference measurements`_ at the end.

What SST optimizes for
======================

SST is designed for producers and consumers that run at the same time and do real work between
steps.  SST reader and writer engines maintain similar or identical semantics to ADIOS
file engines. Several important-to-understand design choices follow from that:

* **Readers pull, writers do not push.**  In ADIOS, a reader decides what it wants
  (``SetSelection``, ``Get``) only after it sees a step's metadata, so a writer cannot know in
  advance which reader rank needs which bytes.  The writer therefore keeps each step's data in a
  queue until every reader cohort has released it, and readers fetch the pieces they asked for.
  There are narrow exceptions to this (see `Geometry locking and preload mode`_), but this
  consideration has large implications on SST behavior and performance.
* **Writers and readers are decoupled.**  The desire for readers not
  to interfere with write-side computation was also a key SST design
  consideration, but it does imply a copy of ``Put()`` data into SST
  buffers.  This allows the
  writer to continue to compute while readers fetch, but necessarily
  involves copy cost
  and memory to hold steps that have not been released yet.
* **Data movement happens in the background.**  Where the fabric supports it (Slingshot/CXI,
  for example), the NIC serves reads from the writer's memory without the writer's
  participation.  Where it does not, a progress thread is started to serve them.  Either way,
  SST attempts to keep transfers from stealing time from the writer's computation.
* **Queueing absorbs rate mismatches.**  ``QueueLimit`` and ``QueueFullPolicy`` decide what
  happens when a reader falls behind.  When writer and reader run at the same speed, the queue
  never fills and these settings have no effect (`Queueing`_).

Getting good performance
========================

Work through these in order when setting up SST for your application.  `Symptoms and fixes`_
covers what to do when something still looks wrong.

#. **Choose the data plane that matches the fabric.**  On Slingshot systems that is ``rdma``
   (libfabric with the CXI provider); on InfiniBand systems it is
   ``ucx``.  If built with RDMA data planes, SST will try to choose
   the best, but it might not build with them by default.  Look at
   CMake output to see which transports were enabled.  Set ``DataTransport``
   explicitly.  While you are getting the job set up, also set ``SST_REQUIRE_DATAPLANE`` to the
   same name on every rank, so that a launch that cannot use it fails instead of falling back to
   ``evpath`` over sockets, which can be 10 to 100 times slower.
#. **Give every launch network access.**  On Slingshot, every writer and reader launch needs a
   VNI, and writer and reader need one in common.  `Platform notes`_ gives the launcher flags.
#. **Bind ranks to cores and choose reader ranks per node from the NIC count.**  On nodes with one
   NIC, one reader rank reached line rate in every measured case.  On nodes with several NICs, use
   at least one reader rank per NIC and, with ``rdma``, set ``FABRIC_IFACE`` per rank, on writers
   and readers, to a NIC in that rank's NUMA domain (`Choosing a NIC per rank`_).  Without it,
   every rank on a node uses the same NIC.
#. **Decide on a queue bound.**  By default the writer queues steps without limit, so a reader that
   falls behind makes writer memory grow.  Choose ``QueueLimit`` and ``QueueFullPolicy`` to fit
   the writer's memory and what the workflow can tolerate (`Queueing`_).

Tuning settings
===============

.. list-table::
   :header-rows: 1
   :widths: 18 14 12 14 42

   * - Setting
     - Set via
     - Data planes
     - Default
     - Effect and trade-off
   * - ``DataTransport``
     - engine parameter
     - all
     - highest priority available
     - Selects the data plane.  Falls back silently (with a warning) if the choice cannot start.
   * - ``SST_REQUIRE_DATAPLANE``
     - environment
     - all
     - unset
     - Makes the stream fail to open instead of falling back.  Use it to validate a system or a
       build.  It must reach every rank.
   * - ``FABRIC_IFACE`` / ``DataInterface``
     - environment / engine parameter
     - rdma
     - Slingshot: first entry of ``SLINGSHOT_DEVICES``.  Otherwise a device from libfabric's
       list
     - Selects the NIC by libfabric domain name, as ``fi_info`` prints it.  Setting it per
       rank to a NIC in the rank's NUMA domain spreads traffic over all NICs.  A NIC in another
       NUMA domain cost about 22% on Frontier.
   * - ``FABRIC_PROGRESS_THREAD``
     - environment
     - rdma
     - on if the provider needs manual data progress
     - No measurable effect on CXI.  Forcing it off on a fabric that needs it can stall transfers.
   * - ``SST_UCX_PROGRESS_THREAD``
     - environment
     - ucx
     - off
     - Needed when UCX falls back to software RMA, which only progresses when the writer's
       worker is driven.
   * - ``QueueLimit``, ``QueueFullPolicy``
     - engine parameters
     - all
     - 0 (unlimited), ``Block``
     - Bound writer memory and decide whether a slow reader makes the writer wait in ``EndStep``
       or loses steps.  No effect when reader and writer keep pace.  See `Queueing`_.
   * - ``SpeculativePreloadMode``, ``SpecAutoNodeThreshold``
     - engine parameters
     - evpath
     - ``AUTO``, 1
     - Sends all data to every reader without waiting for requests when the reader cohort is
       small.  Only the ``evpath`` data plane implements it; others ignore the request.
   * - ``SST_DP_PRELOAD``
     - environment
     - none
     - ignored
     - Formerly enabled learned preload in the ``rdma`` data plane; see
       `Geometry locking and preload mode`_.

Choosing a NIC per rank
=======================

In the ``rdma`` data plane each rank opens its endpoint on the NIC named by its own
``FABRIC_IFACE`` (or the ``DataInterface`` engine parameter), given as a libfabric domain
name.  If neither is set, it uses the first entry of ``SLINGSHOT_DEVICES`` on Slingshot systems,
and otherwise picks from the devices libfabric lists for a supported provider.  A writer rank
publishes that endpoint's address, so its own setting decides which NIC serves its data.  A reader
rank issues its reads from its own endpoint, so its own setting decides which NIC receives them.
Set it on both sides.

This wrapper finds the NUMA domain of the first CPU the rank is bound to and picks a Slingshot
NIC in that domain, spreading ranks by local rank when the domain has several NICs:

.. code-block:: bash

   #!/bin/bash
   # Use a Slingshot NIC attached to the NUMA domain of this rank's first CPU.  When several NICs
   # share that domain, spread ranks over them by local rank.
   cpu=$(taskset -cp $$ | sed 's/.*: //; s/[,-].*//')
   node=$(ls -d /sys/devices/system/cpu/cpu$cpu/node* | head -1); node=${node##*node}
   nics=()
   for d in /sys/class/cxi/cxi*; do
       [ "$(cat $d/device/numa_node)" = "$node" ] && nics+=($(basename $d))
   done
   lr=${PALS_LOCAL_RANKID:-${SLURM_LOCALID:-0}}
   [ ${#nics[@]} -gt 0 ] && export FABRIC_IFACE=${nics[$(( lr % ${#nics[@]} ))]}
   exec "$@"

Launch it in front of the application, for example ``srun ... numanic.sh ./app``, on writer and
reader ranks.  It relies on ranks being bound to cores, for example with
``srun -c7 --threads-per-core=1`` on Frontier and ``mpiexec --cpu-bind depth`` on Aurora.  If no
NIC is in the rank's domain it leaves ``FABRIC_IFACE`` unset.

MPICH's NIC policy (``MPICH_OFI_NIC_POLICY``) applies only to MPI traffic.  SST opens its own
endpoints, so that setting does not move them.

Queueing
========

The writer keeps each step's buffer until every reader has released that step, so writer memory
is about the per-rank step size times the number of steps not yet released.  ``QueueLimit`` bounds
that number, and ``QueueFullPolicy`` decides what happens at the bound:

* ``QueueLimit=0``, the default, sets no bound.  If readers are slower on average, writer memory
  grows by one step for every step they fall behind.
* With ``QueueLimit=N`` and ``QueueFullPolicy=Block``, the default policy, the writer can have N
  unreleased steps and then waits in ``EndStep`` until readers release one.  Writer rank 0 makes
  that decision after the per-step metadata gather, so every writer rank waits.  With
  ``QueueLimit=1`` the writer cannot finish step k+1 until readers have released step k, so the
  two sides run close to lockstep.
* With ``QueueFullPolicy=Discard``, a step that would exceed the bound is dropped: the newest step
  if readers are connected, the oldest if none are.

When this matters:

* If writer and readers keep pace or readers are sufficiently fast, the queue never fills and
  neither setting has any effect.
* If readers are slower on average, no queue setting fixes it.  The
  settings only choose what happens when the ``QueueLimit`` is met:
  the writer waits (``Block``), steps are lost (``Discard``), or writer memory grows (no
  limit).
* If the rates match on average but vary from step to step, a ``QueueLimit`` above 1 lets the
  writer keep going through a slow reader step, at the cost of one more step buffer per step of
  slack.

``ReserveQueueLimit`` and ``FirstTimestepPrecious`` keep steps for readers that connect late and
are not performance settings.  With several reader cohorts, ``StepDistributionMode`` decides
whether every cohort gets every step (``AllToAll``, the default) or steps are spread over them
(``RoundRobin``, ``OnDemand``).  See :ref:`SST Sustainable Staging Transport` for all of these.


Symptoms and fixes
==================

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Symptom
     - Likely cause
     - What to try
   * - Throughput of 100 MB/s or so per node on an HPC network
     - SST fell back to ``evpath`` sockets
     - Look for ``DataPlane ... is not available`` warnings; fix the launch (VNI) or set
       ``SST_REQUIRE_DATAPLANE`` to find out why.
   * - "SLINGSHOT_VNIS is not set" warning from the RDMA data plane
     - Single-node launch without a VNI
     - Frontier: ``srun --network=single_node_vni,job_vni``.  Perlmutter: the same, and submit
       the job with ``sbatch --network=job_vni``.  Aurora: ``mpiexec --single-node-vni``.
   * - More reader ranks per node do not help
     - All ranks share the first NIC
     - Set ``FABRIC_IFACE`` per rank to a NUMA-local NIC.
   * - Readers wait in ``BeginStep`` most of the time
     - The writer is the bottleneck
     - Look at the writer's ``Put``/``EndStep`` time and its computation, not at the network.
   * - Writer waits in ``EndStep``
     - ``QueueLimit`` is set and readers are slower, so the queue is full
     - Speed up readers, raise ``QueueLimit`` if the slowness is intermittent, or allow
       dropping steps with ``QueueFullPolicy=Discard``.  See `Queueing`_.
   * - Writer memory grows step after step
     - No ``QueueLimit`` and readers are slower
     - Set ``QueueLimit`` and choose a ``QueueFullPolicy``.  See `Queueing`_.
   * - The application computes slower while SST is active
     - Background transfers compete for memory bandwidth
     - Expected to some degree; see the interference results in `Reference measurements`_.

Platform notes
==============

What a job needs on each system to use SST's fast data planes.  The software and launch lines are
the ones used for `Reference measurements`_.

Frontier (OLCF)
---------------

.. list-table::
   :widths: 25 75

   * - Software
     - ``cpe/26.03``, ``PrgEnv-gnu``, ``gcc-native/14.2``, ``libfabric/2.3.1``, ``cray-mpich/9.1.0``
       (checked 2026-11-12)
   * - Node
     - AMD EPYC, 4 Slingshot NICs (cxi0 on NUMA 3, cxi1 on 1, cxi2 on 0, cxi3 on 2)
   * - Data plane
     - ``rdma`` (CXI provider)
   * - Launch
     - Writer and reader as separate ``srun`` steps, each with
       ``--network=single_node_vni,job_vni --exclusive``.  Only one job step can use a node at a
       time, so put writer and reader on different nodes.  Use the wrapper in
       `Choosing a NIC per rank`_ on writer and reader ranks.

Aurora (ALCF)
-------------

.. list-table::
   :widths: 25 75

   * - Software
     - Default modules, ``icx``/``icpx``, Aurora MPICH 5.0.0 with PALS, system Mercury with
       Margo 0.24.4 (checked 2026-11-12)
   * - Node
     - 2 x Xeon Max 9470C with HBM, 8 Slingshot NICs (cxi0-3 on socket 0, cxi4-7 on socket 1)
   * - Data planes
     - ``rdma`` (CXI provider).  ``mercury`` with ``SST_MERCURY_PROTOCOL=ofi+cxi`` also works.
   * - Launch
     - Writer and reader as separate ``mpiexec`` launches, each with
       ``--genvall --single-node-vni``.  Launches sharing a node can collide on a CXI PID, so put
       writer and reader on different nodes.  Use the wrapper in `Choosing a NIC per rank`_ on
       writer and reader ranks.

Perlmutter (NERSC)
------------------

.. list-table::
   :widths: 25 75

   * - Software
     - ``cpe/26.03``, ``cpu``, ``PrgEnv-gnu``, ``gcc-native/14``, ``libfabric/2.3.1``,
       ``cray-mpich/9.1.0`` (checked 2026-11-13, CPU nodes only)
   * - Node
     - CPU node, 1 Slingshot NIC (cxi0 on NUMA 2).  One reader rank per node reaches the NIC's
       line rate, so NIC choice and reader rank count make no difference.
   * - Data plane
     - ``rdma`` (CXI provider)
   * - Launch
     - Submit the job with ``sbatch --network=job_vni`` and run writer and reader as separate
       ``srun`` steps on different nodes, each with ``--network=single_node_vni,job_vni
       --exclusive``.  Without the ``sbatch`` option, the second concurrent step fails with "Error
       configuring interconnect".  Without any ``--network`` option, ``SLINGSHOT_VNIS`` is not set
       and the ``rdma`` data plane is unavailable.

Phoenix (Georgia Tech PACE)
---------------------------

.. list-table::
   :widths: 25 75

   * - Software
     - ``gcc/12.3.0``, ``openmpi/4.1.5``, system UCX 1.22.0 (checked 2026-11-14, ``cpu-small``
       nodes)
   * - Node
     - 2 x Xeon Gold 6226 (24 cores, 2 NUMA domains), 1 InfiniBand HCA (``mlx5_0``, 100 Gb/s, on
       NUMA 0).  One reader rank per node reaches the HCA's line rate.
   * - Data plane
     - ``ucx``
   * - Launch
     - Writer and reader as separate ``srun --mpi=pmix --exclusive`` steps on different nodes,
       with ranks bound to cores (``-c3 --cpu-bind=cores`` for 8 ranks per node).

Geometry locking and preload mode
=================================

In some circumstances, SST either assumes or can learn what readers
are going to need what data.  Currently this is only useful with the
``evpath`` data plane.  In particular, if the number of readers is
reader ranks is at most ``SpecAutoNodeThreshold`` (default 1, with
``SpeculativePreloadMode=AUTO``; ``ON`` and ``OFF`` force it either
way), the EVPath dataplane will assume that the readers need data from
all writers and will send the data in EndStep().  Upon arrival the data is queued on the reader and
used to satisfy read requests without the request/response cycle that
would otherwise occur, saving significant latency and keeping the
network busy.  This can also happen if both the reader and the writer
declare that the array geometry in use (array start/count/shape in
Put() and Get()) is fixed or locked.  The writer declares this by
calling ``LockWriterDefinitions()`` on its engine and the reader by
calling ``LockReaderSelections()`` on its engine, both inside a step,
before that step's ``EndStep()``.  After that, the variables, their
shapes and the selections each reader rank uses must stay the same
from step to step.  Only when both sides have locked does the writer
start sending each reader the data it asked for previously, and that
takes effect once the lock has reached the writer, which can be some
steps later.  This is not of current use with any high-performance data
plane.

Reference measurements
======================

These numbers come from ``adios2_iotest`` runs (`Measuring SST performance`_) with synthetic
data, using the software and launch settings in `Platform notes`_ and ADIOS2 master 1918d8237
plus ``adios2_iotest`` timing changes.  They show what the hardware and SST can do, not what any
particular application will see.  Numbers are medians over steps, per node, with 8 writer ranks
per writer node.  "Transfer" is the time readers spend fetching data; "end to end" includes
waiting for the writer.  Sizes and rates use binary units: 1 MiB is 2\ :sup:`20` bytes and 1 GiB
is 2\ :sup:`30` bytes.

Measured rates
--------------

The best rate measured per reader node, with no computation competing for the network.

.. list-table::
   :header-rows: 1
   :widths: 18 32 25 25

   * - System
     - Node network
     - 1 writer node to 1 reader rank
     - Fan-in to 1 reader node
   * - Frontier
     - 4 Slingshot NICs, ``rdma``
     - 19.4 GiB/s
     - 58 GiB/s
   * - Aurora
     - 8 Slingshot NICs, ``rdma``
     - 22.6 GiB/s
     - 120 GiB/s
   * - Perlmutter CPU nodes
     - 1 Slingshot NIC, ``rdma``
     - 22.6 GiB/s
     - 22.6 GiB/s
   * - Phoenix ``cpu-small``
     - 1 InfiniBand HCA (100 Gb/s), ``ucx``
     - 11.5 GiB/s
     - 11.5 GiB/s

On the multi-NIC systems the fan-in rate needs several NUMA-matched reader ranks.

Results by system
-----------------

.. list-table::
   :header-rows: 1
   :widths: 14 30 30 26

   * - System
     - 1 writer node to 1 reader node, 5.7 GiB/step
     - Fan-in, 12 MiB per writer rank
     - Interference
   * - Frontier
     - 1 reader rank: 19.4 GiB/s (NIC bound)
     - 8 or 16 writer nodes.  1 reader rank: 14 GiB/s, 18 GiB/s on a NUMA-local NIC.  16
       NUMA-matched reader ranks: 58 GiB/s.
     - The writer's compute kernel ran 7.5% slower while SST moved data (memory-bound kernel),
       1.5% slower (cache-resident kernel).
   * - Aurora
     - 1 reader rank: ``rdma`` 22.6 GiB/s, ``mercury`` 21.3 GiB/s
     - 8 or 16 writer nodes.  1 reader rank: 22.6 GiB/s (cxi0 is already local to rank 0).
       8 reader ranks: 90 GiB/s.  16 reader ranks: 120 GiB/s.
     - The writer's compute kernel ran 1.5% slower while SST moved data (memory-bound kernel).
   * - Perlmutter
     - 22.6 GiB/s with 1 or 8 reader ranks.  Readers did not wait in ``BeginStep``.
     - 7 writer nodes.  22.6 GiB/s with 1, 8 or 16 reader ranks.
     - Not measured
   * - Phoenix
     - 11.5 GiB/s with 1 or 8 reader ranks, with or without ``SST_UCX_PROGRESS_THREAD=1``.
       Readers did not wait in ``BeginStep``.  Writer ``EndStep`` took at most 0.03 seconds.  The
       same rate at 32 MiB per writer rank.
     - 7 writer nodes.  11.5 GiB/s with 1, 8 or 16 reader ranks.
     - Not measured

Why simple bandwidth tests mislead
----------------------------------

Just a bit of a warning if you're trying to evaluate SST by measuring
what bandwidth it can deliver outside of the context of an HPC
application.
A test that sends data from one writer process to one reader process as fast as it can, with no
computation, measures something real but it might be different from what an application sees.

Infrastructure like queues and data copies cost time when there is nothing to overlap
   Threads and polling that let transfers run alongside computation are pure overhead in a loop
   that only transfers.  SST's generality may limit bandwidth in
   simple circumstances.

Decoupling adds a round trip per step
   Each step is published as metadata, the reader decides what to read, requests it, and the
   transfer follows.  For small steps this latency dominates and the measured "bandwidth" mostly
   reflects the round trip.  This is particularly noticeable with
   high-latency data planes.

One reader rank sees one NIC
   A single reader process uses a single NIC, so a one-to-one test cannot show what spreading
   ranks over a node's NICs does.  On Frontier, 16 reader ranks spread over the 4 NICs read about
   58 GiB/s per node, against about 14 GiB/s for one rank on the default NIC.

Interference depends on the application
   Background transfers compete with the application for memory bandwidth, not only for the
   network.  A cache-resident compute kernel slowed by about 1.5% on Frontier while SST moved 5.7
   GiB per step out of the node; a kernel that streams through 256 MiB slowed by about 7.5%.  On
   Aurora, whose nodes have HBM, the memory-bound kernel slowed by about 1.5%.

Queue settings and preload target specific situations
   ``QueueLimit`` and ``QueueFullPolicy`` matter when readers and writers run at different
   speeds.  Preload, sending data before the reader asks for it, is only implemented by the
   ``evpath`` data plane: speculatively for small reader cohorts, and for a fixed read pattern
   after the reader calls ``LockReaderSelections()``.  It has no effect with the ``rdma``,
   ``ucx``, ``mpi`` or ``mercury`` data planes.

Measuring SST performance
-------------------------

``adios2_iotest`` (``source/utils/adios_iotest``) runs a writer and a reader described by a small
configuration file.  The commands useful for SST measurements are:

``work <iterations> [MB]``
   A fixed amount of computation over a buffer of the given size in MiB (default 8).  Its time is
   logged per rank, so a slowdown caused by background transfers shows up directly.  Use a buffer
   larger than the last-level cache to model a memory-bound application.

``-t``
   Writes per-step, per-rank times to ``app<id>_perf.csv`` (rows ``write``, ``endstep``, ``read``,
   ``beginstep``) and ``app<id>_work.csv``.  ``read`` includes the wait in ``BeginStep``;
   ``read`` minus ``beginstep`` is the transfer.

``-F``
   Locks the read pattern (``LockReaderSelections()``) and the writer definitions after the first
   step.

An example configuration with 8 writer ranks at 730 MiB each, read by any number of reader ranks:

.. code-block:: text

   group w
     array double a 1 765460480 X
   group r
     array double a 1 765460480 X
   app 1
     steps 12
     work 40 256
     write s.bp w
   app 2
     steps 12
     read next s.bp r -1

with an XML file selecting ``SST`` and ``DataTransport`` for the ``w`` and ``r`` groups, and the
two applications started as separate launches with ``-a 1 -s -d 8 -t`` and ``-a 2 -s -d <readers>
-t``.  Discard the first step, which includes connection setup, and report medians.
