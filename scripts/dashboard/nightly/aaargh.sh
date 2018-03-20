#!/bin/bash

# Source any site-specific variables or scripts
if [ -f ${HOME}/.dashboard ]
then
  source ${HOME}/.dashboard
fi

function log()
{
  local TIMESTAMP=$(date +"%Y%m%dT%T.%N")
  echo "${TIMESTAMP}  " "$@" | tee -a Logs/aaargh.log
}

mkdir -p ${HOME}/Dashboards/ADIOS2
cd ${HOME}/Dashboards/ADIOS2

mkdir -p Logs
rm -f Logs/aaargh.log

module purge
module load cmake

CTEST=$(which ctest)

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

# Make sure we have a newer binutils available

source /opt/rh/devtoolset-7/enable

log "Running Serial GCC7"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-nompi.cmake 2>&1 1>Logs/aaargh-gcc7-nompi.log

log "Running Serial Intel17"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel17-nompi.cmake 2>&1 1>Logs/aaargh-intel17-nompi.log

log "Running Serial Intel18"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel18-nompi.cmake 2>&1 1>Logs/aaargh-intel18-nompi.log

# Now run the MPI tests

log "Running GCC7 MPICH"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-mpi.cmake \
  -DMPI_NAME=MPICH -DMPI_MODULE=mpich 2>&1 1>Logs/aaargh-gcc7-mpich.log

log "Running Intel17 OpenMPI"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel17-mpi.cmake \
  -DMPI_NAME=OpenMPI -DMPI_MODULE=openmpi3 2>&1 1>Logs/aaargh-intel17-openmpi.log

log "Running Intel18 IntelMPI"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel18-mpi.cmake \
  -DMPI_NAME=IntelMPI -DMPI_MODULE=impi 2>&1 1>Logs/aaargh-intel18-impi.log

log "Running Clang5 NoMPI AddressSanitizer"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-clang5-nompi-asan.cmake 2>&1 1>Logs/aaargh-clang5-nompi-asan.log

log "Running Clang5 NoMPI MemorySanitizer"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-clang5-nompi-msan.cmake 2>&1 1>Logs/aaargh-clang5-nompi-msan.log

# Do static analysis
log "Running Coverity Static Analysis"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-mpi-coverity.cmake 2>&1 1>Logs/aaargh-gcc7-mpi-coverity.cmake.log
