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

export ZFP_ROOT=${HOME}/Dashboards/Support/zfp/0.5.3
export SZ_ROOT=${HOME}/Dashboards/Support/sz/master

for F in gcc7 intel17 intel18 gcc7-mpich intel17-impi intel18-openmpi gcc7-gcov gcc7-mpich-gcov gcc7-valgrind clang5-asan clang5-msan gcc7-coverity
do
  log "Running ${F}"
  ${CTEST} -VV -S ${SCRIPT_DIR}/aaargh-${F}.cmake 2>&1 1>Logs/aaargh-${F}.log
done
