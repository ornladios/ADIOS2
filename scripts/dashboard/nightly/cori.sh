#!/bin/bash

function log()
{
  local TIMESTAMP=$(date +"%Y%m%dT%T.%N")
  echo "${TIMESTAMP}  " "$@" | tee -a Logs/cori.log
}

mkdir -p ${HOME}/dashboards/cori/adios2
cd ${HOME}/dashboards/cori/adios2
mkdir -p Logs
rm -f Logs/cori.log

module load git

CTEST=${HOME}/dashboards/cori/support/CMake/install/v3.9.5/bin/ctest

if [ ! -d Source/.git ]
then
  git clone https://github.com/ornladios/adios2.git Source
else
  pushd Source
  git fetch --all -p
  git checkout -f master
  git pull --ff-only
  popd
fi
SCRIPT_DIR=${PWD}/Source/scripts/dashboard/nightly

# Run the configure and build steps for the MPI tests
log "Running Parallel GCC Build"
${CTEST} -VV -S ${SCRIPT_DIR}/cori-gcc-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_fresh=ON \
  -Ddashboard_do_checkout=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON 2>&1 1>Logs/cori-gcc-mpich.log

log "Running Parallel Intel Build"
${CTEST} -VV -S ${SCRIPT_DIR}/cori-intel-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_fresh=ON \
  -Ddashboard_do_checkout=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON 2>&1 1>Logs/cori-intel-mpich.log

log "Running Parallel Cray Build"
${CTEST} -VV -S ${SCRIPT_DIR}/cori-cray-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_fresh=ON \
  -Ddashboard_do_checkout=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON 2>&1 1>Logs/cori-cray-mpich.log

# Now run the MPI tests in a batch job
log "Submitting Parallel Tests"
JOBID=$(sbatch --array=1-3 ${SCRIPT_DIR}/cori-mpich-tests.slurm | awk '{print $4}')
sleep 30
while true
do
  NJOBS=$(sacct -n -P --delimiter ' ' -s CF,CG,PD,R,RS -j ${JOBID} | wc -l)
  log "Test jobs active in queue for job array ${JOBID}: ${NJOBS}"
  if [ ${NJOBS} -eq 0 ]
  then
    break
  fi
  sleep 30
done

# Finaly submit the test results from the batch job
log "Submitting Parallel GCC Test Results"
${CTEST} -VV -S ${SCRIPT_DIR}/cori-gcc-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit_only=ON 2>&1 1>>Logs/cori-gcc-mpich.log

log "Submitting Parallel Intel Test Results"
${CTEST} -VV -S ${SCRIPT_DIR}/cori-intel-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit_only=ON 2>&1 1>>Logs/cori-intel-mpich.log

log "Submitting Parallel Cray Test Results"
${CTEST} -VV -S ${SCRIPT_DIR}/cori-cray-mpich.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit_only=ON 2>&1 1>>Logs/cori-cray-mpich.log
