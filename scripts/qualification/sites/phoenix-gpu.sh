# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

# The Kokkos CUDA and Python GPU array tests on one Phoenix RTX 6000 node.  phoenix.sh runs the
# data planes.  On embers these nodes start much sooner than the A100 nodes.
ACCOUNT="${ACCOUNT:-gts-$USER}"
SITE_ROOT="/storage/scratch1/${HOME#/storage/home/hcoda1/}/adios2-qualification"

KOKKOS_VERSION=4.4.01
KOKKOS_ARGS=(-DKokkos_ENABLE_CUDA=ON -DKokkos_ARCH_TURING75=ON -DKokkos_ARCH_SKX=ON)

CMAKE_ARGS=(-DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -DADIOS2_USE_Fortran=OFF
    -DADIOS2_USE_Kokkos=ON -DCMAKE_CUDA_ARCHITECTURES=75 -DADIOS2_USE_Python=ON
    -DMPIEXEC_MAX_NUMPROCS=6
    "-DMPIEXEC_EXTRA_FLAGS=--mpi=pmix --overlap")
REQUIRE_CACHE=(ADIOS2_USE_Kokkos ADIOS2_USE_Python)

GPU_TESTS='Kokkos|BPWRLayout|MemorySpace|Api\.Python\.BPWriteReadArrays'

site_env()
{
    command -v module >/dev/null || source /etc/profile
    # The newest CUDA modules are newer than this Kokkos supports.
    module load gcc/12.3.0 openmpi/4.1.5 cmake/3.30.2 cuda/12.6.1 python/3.11.9
    # There is no cupy module, and the torch modules have no torch for this Python.
    local venv="$ROOT/venv"
    if [ ! -e "$venv/done" ]; then
        python3 -m venv "$venv" &&
            "$venv/bin/pip" install -q numpy mpi4py cupy-cuda12x &&
            "$venv/bin/pip" install -q torch --index-url https://download.pytorch.org/whl/cu126 &&
            touch "$venv/done" ||
            return 1
    fi
    CMAKE_ARGS+=("-DPython_EXECUTABLE=$venv/bin/python")
    # The Python GPU array test fails rather than falling back to the CPU.
    export ADIOS2_TEST_EXPECT_CUDA=1
    # The build node has no GPU driver, so the linker gets the driver stub under the driver's
    # name.  The executables still load the real driver on the GPU node.
    mkdir -p "$ROOT/cuda-stub" &&
        ln -sf "$CUDA_HOME/lib64/stubs/libcuda.so" "$ROOT/cuda-stub/libcuda.so.1" || return 1
    CMAKE_ARGS+=("-DCMAKE_EXE_LINKER_FLAGS=-Wl,-rpath-link,$ROOT/cuda-stub")
    CMAKE_ARGS+=(-DMPIEXEC_EXECUTABLE="$(command -v srun)")
    # The Kokkos tests compile with nvcc_wrapper, which gets MPI from mpicxx this way.
    export NVCC_WRAPPER_DEFAULT_COMPILER=mpicxx
    KOKKOS_ARGS+=("-DCMAKE_CXX_COMPILER=$ROOT/deps/kokkos-$KOKKOS_VERSION/bin/nvcc_wrapper")
}

# See phoenix.sh for why a job builds.  The build runs on a CPU node, so the GPU job is short
# enough to start soon on embers.
BUILD_IN_JOB=1
site_submit()
{
    local build_job
    build_job="$(sbatch --parsable -A "$ACCOUNT" -J adios2-qualify-build -q embers -N 1 -n 1 \
        -c 24 -t 01:30:00 -o "$ROOT/build-%j.out" --wrap="BUILD_J=24 $SELF $site build")" ||
        return 1
    echo "Submitted build job $build_job"
    # These nodes allow six CPUs per GPU.
    sbatch -A "$ACCOUNT" -J adios2-qualify -q embers -N 1 --ntasks-per-node=6 \
        --gres=gpu:rtx_6000:1 -t 00:20:00 --dependency=afterok:"$build_job" \
        -o "$ROOT/run-%j.out" --wrap="$SELF $site run"
}
