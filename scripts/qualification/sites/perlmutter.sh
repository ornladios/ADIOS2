# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

ACCOUNT="${ACCOUNT:-m3976_g}"
SITE_ROOT="$SCRATCH/adios2-qualification"

KOKKOS_VERSION=4.4.01
KOKKOS_ARGS=(-DKokkos_ENABLE_CUDA=ON -DKokkos_ARCH_AMPERE80=ON -DKokkos_ARCH_ZEN3=ON)

# See frontier.sh for the launch setup, which has only been tried there.
CMAKE_ARGS=(-DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=CC -DADIOS2_USE_Fortran=OFF
    -DADIOS2_USE_Kokkos=ON -DCMAKE_CUDA_ARCHITECTURES=80
    -DADIOS2_RUN_MPI_MPMD_TESTS=OFF -DMPIEXEC_EXECUTABLE="$(command -v srun)" -DMPIEXEC_MAX_NUMPROCS=8
    "-DMPIEXEC_EXTRA_FLAGS=--network=single_node_vni,job_vni --overlap")
REQUIRE_CACHE=(ADIOS2_SST_HAVE_CRAY_CXI ADIOS2_USE_Kokkos)

# See frontier.sh for why the MPI data plane is left out.
DATAPLANES="rdma"
QUAL_SCHED=slurm
LAUNCH_NODES=1
CTEST_J=1
GPU_TESTS='Kokkos|BPWRLayout|MemorySpace'
EXCLUDE='Engine\.Staging\.TestThreads'

site_env()
{
    command -v module >/dev/null || source /etc/profile
    # The default CUDA 13 is newer than this Kokkos supports.
    module load PrgEnv-gnu gcc-native/13.2 cudatoolkit/12.9 cmake libfabric
    # Otherwise CC links GPU-aware MPI, which ADIOS2 does not use and which needs libcudart.
    module unload craype-accel-nvidia80
    # The Kokkos tests compile with nvcc_wrapper, which gets MPI from the Cray wrapper this way.
    export NVCC_WRAPPER_DEFAULT_COMPILER=CC
    KOKKOS_ARGS+=("-DCMAKE_CXX_COMPILER=$ROOT/deps/kokkos-$KOKKOS_VERSION/bin/nvcc_wrapper")
}

site_submit()
{
    sbatch -A "$ACCOUNT" -J adios2-qualify -C gpu -q regular -N 4 --gpus-per-node=4 \
        -t 02:00:00 -o "$ROOT/run-%j.out" --wrap="ACCOUNT=$ACCOUNT $SELF $site run"
}
