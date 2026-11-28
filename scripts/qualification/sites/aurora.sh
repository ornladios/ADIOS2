# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

# DAOS is not part of this build.  aurora-daos.sh runs it, because its tests launch with
# --no-vni.
ACCOUNT="${ACCOUNT:-Optimizing_ADIOS_IO}"
SITE_ROOT="$HOME/adios2-qualification"

# PALS does not forward the environment by default.  It rejects --ppn with MPMD launches, and
# without it MPMD ranks alternate between nodes.  A launch on one node gets no VNI, so RDMA
# refuses it, which is why launch.sh gives each launch two nodes.  KillReaders3Max has four
# launches running at once.  Kokkos comes from the module, which is 5.x and untested by upstream
# CI.
CMAKE_ARGS=(-DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx -DADIOS2_USE_Fortran=OFF
    -DADIOS2_USE_Kokkos=ON -DADIOS2_USE_DAOS=OFF -DADIOS2_RUN_MPI_MPMD_TESTS=ON -DMPIEXEC_MAX_NUMPROCS=8
    "-DMPIEXEC_EXTRA_FLAGS=-genvall --cpu-bind numa" -DADIOS2_USE_Mercury=ON)
REQUIRE_CACHE=(ADIOS2_SST_HAVE_CRAY_CXI ADIOS2_USE_Kokkos ADIOS2_USE_Mercury)

# Mercury defaults to tcp.  The system Mercury needs Boost headers and has no Margo, so Margo is
# built into deps/ as described in README.md.
DATAPLANES="rdma mercury mpi"
# A launch on one node gets no VNI, so rdma cannot run single-rank separate launches.
SKIP_rdma='Staging\.(MultiStreamBiDir\.1x1|TimeoutReader\.1x1|LatestReader\.1x1|LatestReaderHold\.1x1|DiscardWriter\.1x1|RoundRobinDistribution\.1x1x3|AllToAllDistribution\.1x1x3)\.SST'
# MPI_Comm_connect fails between separate launches.
SKIP_mpi='Staging\.(MultiStreamBiDir\.2x2|DelayedReader_3x5|KillReadersSerialized\.3x2|KillReaders3Max\.3x6|KillWriter_2x2|KillWriterTimeout_2x2|PreciousTimestep\.3x2|PreciousTimestepDiscard\.3x2|RoundRobinDistribution\.1x1x3|AllToAllDistribution\.1x1x3)\.SST'
# Every rank of 3x5BiDir is both a writer and a reader, and the MPI data plane makes blocking MPI
# calls on the network thread its streams share, so the two sides wait on each other.
SKIP_mpi="$SKIP_mpi|Staging\.3x5BiDir\.SST"
QUAL_SCHED=pals
LAUNCH_NODES=2
CTEST_J=1
GPU_TESTS='Kokkos|BPWRLayout|MemorySpace'

site_env()
{
    command -v module >/dev/null || source /etc/profile
    module use /soft/modulefiles
    module load cmake kokkos/5.1.1-sycl argobots/1.2 boost/1.88.0
    export MARGO_ROOT="$ROOT/deps/margo-0.24.4"
    export PKG_CONFIG_PATH="$ROOT/deps/pkgconfig:$PKG_CONFIG_PATH"
    export LD_LIBRARY_PATH="$MARGO_ROOT/lib64:$LD_LIBRARY_PATH"
    export SST_MERCURY_PROTOCOL=ofi+cxi
}

site_submit()
{
    qsub -A "$ACCOUNT" -N adios2-qualify -q capacity -l select=8 -l walltime=02:00:00 \
        -l filesystems=home:flare -j oe -o "$ROOT/" -- "$SELF" "$site" run
}
