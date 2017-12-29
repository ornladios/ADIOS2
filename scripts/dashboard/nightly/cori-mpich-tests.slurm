#!/bin/bash
#SBATCH -A m2886
#SBATCH -p debug
#SBATCH -N 2
#SBATCH -t 0:30:00
#SBATCH -n 8
#SBATCH -J adios_nightly
#SBATCH -C knl,quad,cache

case ${SLURM_ARRAY_TASK_ID} in
  1) CTEST_COMPILER=gcc
     ;;
  2) CTEST_COMPILER=intel
     ;;
  3) CTEST_COMPILER=cray
     ;;
  *) echo "Error: Unsupported SLURM_ARRAY_TASK_ID: ${SLURM_ARRAY_TASK_ID}"
     exit
     ;;
esac

echo "SLURM_ARRAY_TASK_ID: ${SLURM_ARRAY_TASK_ID}"
echo "CTEST_COMPILER: ${CTEST_COMPILER}"

CTEST=${HOME}/dashboards/cori/support/CMake/install/v3.9.5/bin/ctest
SCRIPT_DIR=${PWD}/Source/scripts/dashboard/nightly
${CTEST} -VV -S ${SCRIPT_DIR}/cori-${CTEST_COMPILER}-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit=OFF 2>&1 1>>Logs/cori-${CTEST_COMPILER}-mpich.log
