# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

# Compute nodes cannot write to home, so everything lives on Lustre.
: "${ACCOUNT:?set ACCOUNT to your Frontier project, for example csc143}"
SITE_ROOT="/lustre/orion/${ACCOUNT,,}/scratch/$USER/adios2-qualification"

# 4.7.03 is the first Kokkos release that supports ROCm 7.
KOKKOS_VERSION=4.7.04
KOKKOS_ARGS=(-DCMAKE_CXX_COMPILER=hipcc -DKokkos_ENABLE_HIP=ON -DKokkos_ARCH_VEGA90A=ON
    -DKokkos_ARCH_ZEN3=ON -DKokkos_ENABLE_HIP_RELOCATABLE_DEVICE_CODE=OFF)

# srun has no MPMD colon syntax outside heterogeneous jobs, so writer and reader are separate
# steps.  Only one step at a time can use a node, so launch.sh gives each its own.
CMAKE_ARGS=(-DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=CC -DADIOS2_USE_Fortran=OFF
    -DADIOS2_USE_Kokkos=ON -DCMAKE_HIP_ARCHITECTURES=gfx90a
    -DADIOS2_RUN_MPI_MPMD_TESTS=OFF -DMPIEXEC_EXECUTABLE="$(command -v srun)" -DMPIEXEC_MAX_NUMPROCS=8
    "-DMPIEXEC_EXTRA_FLAGS=--network=single_node_vni,job_vni --overlap")
REQUIRE_CACHE=(ADIOS2_SST_HAVE_CRAY_CXI ADIOS2_USE_Kokkos)

# The MPI data plane needs MPI_Comm_connect between separate launches, which hits an assertion
# in Cray MPICH, and srun cannot launch MPMD here.  Its failures also leave steps behind.
DATAPLANES="rdma"
QUAL_SCHED=slurm
LAUNCH_NODES=1
CTEST_J=1
GPU_TESTS='Kokkos|BPWRLayout|MemorySpace'
EXCLUDE='Engine\.Staging\.TestThreads'

site_env()
{
    command -v module >/dev/null || source /etc/profile
    # The GPU support library in CPE 26.03's cray-mpich needs ROCm 7.
    module load cpe/26.03 PrgEnv-gnu gcc-native/14.2 craype-accel-amd-gfx90a rocm/7.0.2 \
        cmake libfabric
    # The Kokkos tests compile with hipcc, which does not get MPI from the Cray wrappers.
    export HIPCC_COMPILE_FLAGS_APPEND="-I$MPICH_DIR/include"
    export HIPCC_LINK_FLAGS_APPEND="-L$MPICH_DIR/lib -lmpi"
}

# A killed srun can leave its step behind for a while, and that blocks the next launch.
site_settle()
{
    local wait
    for ((wait = 0; wait < 60; wait++)); do
        squeue -h -s -j "$SLURM_JOB_ID" -o %i | grep -q -v -E '\.(batch|extern|interactive)$' || return
        sleep 2
    done
}

site_submit()
{
    sbatch -A "$ACCOUNT" -J adios2-qualify -N 4 -t 02:00:00 -q debug -o "$ROOT/run-%j.out" \
        --wrap="ACCOUNT=$ACCOUNT $SELF $site run"
}
