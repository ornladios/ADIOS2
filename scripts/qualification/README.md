<!--
SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors

SPDX-License-Identifier: Apache-2.0
-->

# Platform qualification

Tests for features CI cannot reach: SST data planes on Slingshot, GPU memory
paths, and DAOS. These run by hand on Frontier, Perlmutter, Aurora and Phoenix, on a
release candidate and whenever a change touches one of those paths. Results fill
in the table in `docs/NewRelease.md.tmpl`.

## Running

From a checkout of the commit under test, on a login node:

```bash
ACCOUNT=<project> scripts/qualification/qualify.sh frontier build
ACCOUNT=<project> scripts/qualification/qualify.sh frontier submit
```

The site is `frontier`, `perlmutter`, `aurora`, `aurora-daos` (the DAOS engine tests),
`phoenix` (PACE at Georgia Tech) or `phoenix-gpu` (the Kokkos CUDA and Python GPU
array tests on a Phoenix RTX 6000). `ACCOUNT` is required on Frontier and has a default on the others. Results land in
`<root>/<site>/results/<timestamp>-<commit>/`, with `SUMMARY.txt` as the short
version. `<root>` is set by the site file or by `QUAL_ROOT`.

On Phoenix (both site files) the login nodes are too small to build on, so skip `build`: `submit`
builds in the job.

Then compare with the last qualification on that machine:

```bash
scripts/qualification/compare.py <baseline-results> <new-results>
```

It exits nonzero if anything regressed. Keep the results directory of each
release as the next baseline. The first run on a machine has nothing to compare
with, so read its failures by hand and decide what the baseline is.

## What a run does

1. **Control.** Checks that a test runs, then requires a data plane that does not
   exist and checks that the test fails with that error. This proves
   `SST_REQUIRE_DATAPLANE` reaches the ranks. Without that, SST picks its own data
   plane and every result below proves nothing.
2. **`sst-<dataplane>`.** For each data plane, a quick probe, then the staging
   tests with that data plane required. A test cannot pass on a fallback. A data
   plane that fails the probe is reported as `UNAVAILABLE`, which `compare.py`
   counts as a regression if it worked in the baseline. evpath is not a phase,
   since CI runs these tests with it.
   A site can set `SKIP_<dataplane>` to a ctest regex of tests that cannot pass
   there, such as separate launches under `mpi`, so they do not spend their
   timeouts on every run. `SUMMARY.txt` says how many were skipped, and the RC
   table still lists what is not covered.
3. **`recheck-<dataplane>`.** The tests that failed in a data plane phase, rerun
   under evpath on the same nodes. A test that passes there points at the data
   plane, one that fails there too points at the test or the machine.
4. **`gpu`.** The Kokkos tests and the GPU array-layout and memory-space tests on
   the machine's GPUs.

Separately launched writers and readers go through `launch.sh`, which gives each
launch nodes of its own, so these tests run one at a time. That also makes the
traffic cross the network. It is needed on Slingshot for two reasons:

- On Frontier only one job step at a time can use a node. A second one fails with
  `Error configuring interconnect`.
- On Aurora, separate launches on the same node can collide on a PID on the job
  VNI. The driver reports `EEXIST` and libfabric reports it as `No space left on
  device`. This matches the per-profile PID tables in the SHS 14 driver, which
  shs-cxi-driver f681a7e63c6f replaces. Disjoint nodes avoid it.

Phoenix launches with `srun --mpi=pmix`, since Open MPI's `mpirun` cannot start
its daemons on a second node there. An MPMD `srun` step hangs, so MPMD tests are
off, and with them the mpi data plane. Separate launches get their own nodes, so
UCX traffic crosses InfiniBand. A leftover step does not block a node there, and
`squeue` can take tens of seconds, so `launch.sh` does not wait for leftover steps.
Phoenix has no libfabric, so no rdma data plane.

On Aurora a launch on one node gets no VNI, so RDMA refuses single-rank separate
launches, and those tests are expected to fail. MPMD launches do not go through
placement.

## What it does not cover

- **Raw CUDA pointers.** The tests were removed in b8ffbb91b. The examples do not
  verify the data they read, so they are not used here.
- **UCX on Slingshot.** It has no Slingshot transport, so only Phoenix runs it.
- **Mercury on Frontier and Perlmutter.** They have no Mercury or Margo installed.
- **Scale.** Runs use four to eight nodes.
- **Performance.** Only correctness is checked.

## Mercury on Aurora

The system Mercury has no Margo, so build Margo 0.24.4 into
`<root>/aurora/deps/margo-0.24.4` once, with CMake, gcc and the `argobots/1.2` and
`boost/1.88.0` modules loaded. The Mercury headers need Boost. The system Mercury
also has no `mercury.pc`, which `margo.pc` requires, so put one in
`<root>/aurora/deps/pkgconfig` with prefix `/usr`, libdir `/usr/lib64`, and
`Libs: -lmercury -lna -lmercury_util`.

## DAOS

DAOS runs as its own site, `aurora-daos`, since its tests launch with `--no-vni`,
which RDMA cannot use. It needs the DAOS tests (`-DADIOS2_RUN_DAOS_TESTS=ON`) in
the tree, and runs `ctest -L DAOS` on one debug node in a new container in the
`DAOS_POOL` pool (default the project's), destroyed afterwards. The build and its
test output stay on home, because the `daos/base` environment has hung BP5 on
Lustre.

## Trying it out

The `local` site runs the same steps on any MPI build:

```bash
QUAL_BUILD=$PWD/build scripts/qualification/qualify.sh local run
```

## Notes

The `mpi` data plane connects with `MPI_Comm_connect`, so tests that launch writer
and reader as separate jobs need an MPI that can connect separate jobs. None so far
can: MPICH on Aurora fails in `MPI_Comm_connect`, Cray MPICH on Frontier hits an
assertion, and Open MPI on a workstation times out. Within one MPMD launch it
works, so Aurora runs it and its separate-launch failures are expected. Frontier
and Perlmutter leave it out, since `srun` cannot launch MPMD there. Aurora also
skips `3x5BiDir`, where every rank is both a writer and a reader: the data plane
makes blocking MPI calls on the network thread its streams share, so the two sides
wait on each other.

The site files set `MPIEXEC_MAX_NUMPROCS`, which otherwise defaults to the core count
of the node that configures, so tests would ask for more ranks than the job has.

A test that hangs holds a slot until its timeout, 1000 seconds under `srun`. If
the job runs out of walltime, that phase has a log but no xml, and `compare.py`
reports it as having no results.
