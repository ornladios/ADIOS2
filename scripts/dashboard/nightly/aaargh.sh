#!/bin/bash

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

# First run the serial tests
log "Running Serial GCC5"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc5-nompi.cmake 2>&1 1>Logs/aaargh-gcc5-nompi.log

log "Running Serial GCC7"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-nompi.cmake 2>&1 1>Logs/aaargh-gcc7-nompi.log

log "Running Serial Intel17"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel17-nompi.cmake 2>&1 1>Logs/aaargh-intel17-nompi.log

# Now run the MPI tests
log "Running GCC7 IntelMPI"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-mpi.cmake \
  -DMPI_NAME=IntelMPI -DMPI_MODULE=impi 2>&1 1>Logs/aaargh-gcc7-impi.log

log "Running GCC7 MPICH"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-mpi.cmake \
  -DMPI_NAME=MPICH -DMPI_MODULE=mpich 2>&1 1>Logs/aaargh-gcc7-mpich.log

log "Running GCC7 MVAPICH2"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-gcc7-mpi.cmake \
  -DMPI_NAME=MVAPICH2 -DMPI_MODULE=mvapich2 2>&1 1>Logs/aaargh-gcc7-mvapich2.log

log "Running Intel17 IntelMPI"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel17-mpi.cmake \
  -DMPI_NAME=IntelMPI -DMPI_MODULE=impi 2>&1 1>Logs/aaargh-intel17-impi.log

log "Running Intel17 MPICH"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel17-mpi.cmake \
  -DMPI_NAME=MPICH -DMPI_MODULE=mpich 2>&1 1>Logs/aaargh-intel17-mpich.log

log "Running Intel17 MVAPICH2"
${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-intel17-mpi.cmake \
  -DMPI_NAME=MVAPICH2 -DMPI_MODULE=mvapich2 2>&1 1>Logs/aaargh-intel17-mvapich2.log
