#!/bin/bash

function log()
{
  local TIMESTAMP=$(date +"%Y%m%dT%T.%N")
  echo "${TIMESTAMP}  " "$@" | tee -a summitdev.log
}

mkdir -p ${HOME}/dashboards/summitdev/adios2
cd ${HOME}/dashboards/summitdev/adios2

rm -f summitdev.log

module purge
module load git cmake lsf-tools

CTEST=$(which ctest)

if [ ! -d source/.git ]
then
  git clone https://github.com/ornladios/adios2.git source
else
  pushd source
  git fetch --all -p
  git checkout -f master
  git pull --ff-only
  popd
fi
SCRIPT_DIR=${PWD}/source/scripts/dashboard/nightly

# First run the serial tests
log "Running Serial GCC"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-gcc-nompi.cmake 2>&1 1>summitdev-gcc-nompi.log
log "Running Serial XL"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-xl-nompi.cmake 2>&1 1>summitdev-xl-nompi.log
log "Running Serial XL"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-pgi-nompi.cmake 2>&1 1>summitdev-pgi-nompi.log

# Now run the configure and build steps for the MPI tests
log "Running Parallel GCC Build"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-gcc-spectrum.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_fresh=ON \
  -Ddashboard_do_checkout=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON 2>&1 1>summitdev-gcc-spectrum.log

log "Running Parallel XL Build"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-xl-spectrum.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_fresh=ON \
  -Ddashboard_do_checkout=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON 2>&1 1>summitdev-xl-spectrum.log

log "Running Parallel PGI Build"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-pgi-spectrum.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_fresh=ON \
  -Ddashboard_do_checkout=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON 2>&1 1>summitdev-pgi-spectrum.log

# Now run the MPI tests in a batch job
log "Submitting Parallel Tests"
JOBID=$(bsub -J "adios2_nightly[1-3]" ${SCRIPT_DIR}/summitdev-spectrum-tests.lsf | awk '{print $2}' | sed 's|<\([0-9]*\)>|\1|')
while true
do
  NJOBS=$(bjobs 2>/dev/null | grep "^${JOBID} " | wc -l) 
  log "Test jobs active in queue for job array ${JOBID}: ${NJOBS}"
  if [ ${NJOBS} -eq 0 ]
  then
    break
  fi
  sleep 30
done

# Finaly submit the test results from the batch job
log "Submitting Parallel GCC Test Results"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-gcc-spectrum.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit_only=ON 2>&1 1>>summitdev-gcc-spectrum.log

log "Submitting Parallel XL Test Results"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-xl-spectrum.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit_only=ON 2>&1 1>>summitdev-xl-spectrum.log

log "Submitting Parallel PGI Test Results"
${CTEST} -VV -S ${SCRIPT_DIR}/summitdev-pgi-spectrum.cmake \
  -Ddashboard_full=OFF \
  -Ddashboard_do_test=ON \
  -Ddashboard_do_submit_only=ON 2>&1 1>>summitdev-pgi-spectrum.log
