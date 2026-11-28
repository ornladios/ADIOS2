# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Sourced by qualify.sh, which reads these settings and sets site, cmd, ROOT and SELF.
# shellcheck shell=bash disable=SC1091,SC2034,SC2154

# PACE Phoenix at Georgia Tech: InfiniBand, Open MPI under Slurm.  embers is the free QOS; its
# jobs can be preempted.
ACCOUNT="${ACCOUNT:-gts-$USER}"
SITE_ROOT="/storage/scratch1/${HOME#/storage/home/hcoda1/}/adios2-qualification"

# See frontier.sh for the launch setup.  mpirun cannot start its daemons on a second node here,
# and an MPMD srun step hangs.
CMAKE_ARGS=(-DCMAKE_C_COMPILER=mpicc -DCMAKE_CXX_COMPILER=mpicxx -DADIOS2_USE_Fortran=OFF
    -DADIOS2_USE_UCX=ON -DADIOS2_RUN_MPI_MPMD_TESTS=OFF -DMPIEXEC_MAX_NUMPROCS=8
    "-DMPIEXEC_EXTRA_FLAGS=--mpi=pmix --overlap")

# No libfabric here, so no rdma data plane.  Without MPMD every mpi test is a separate launch,
# and MPI_Comm_connect fails between those.
DATAPLANES="ucx"
QUAL_SCHED=slurm
LAUNCH_NODES=1
# A leftover step does not block a node here, and squeue can take tens of seconds, so launches
# do not wait for one.
STEP_WAIT=0
CTEST_J=1

site_env()
{
    command -v module >/dev/null || source /etc/profile
    # openmpi/4.1.5 links the system UCX, which has the InfiniBand transports.  The UCX that
    # the 5.0 modules link has only tcp and shared memory.
    module load gcc/12.3.0 openmpi/4.1.5 cmake/3.30.2
    CMAKE_ARGS+=(-DMPIEXEC_EXECUTABLE="$(command -v srun)")
}

# Login nodes cap a user at 4 GB, which the build exceeds, so the job builds first.
BUILD_IN_JOB=1
site_submit()
{
    sbatch -A "$ACCOUNT" -J adios2-qualify -q embers -N 4 --ntasks-per-node=8 -t 03:00:00 \
        -o "$ROOT/run-%j.out" --wrap="BUILD_J=24 $SELF $site build && $SELF $site run"
}
