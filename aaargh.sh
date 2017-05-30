#!/bin/bash

BASEDIR=$(readlink -f $(dirname ${BASH_SOURCE}))
cd ${BASEDIR}

echo "Updating scripts..."
git pull --ff-only
cd ${HOME}
mkdir -p ${BASEDIR}/../Logs

DASHBOARD_CONFIGS="System GCC_IMPI GCC_MPICH GCC_MVAPICH2 GCC_NoMPI GCC_OpenMPI Intel_IMPI Intel_MPICH Intel_MVAPICH2 Intel_NoMPI Intel_OpenMPI"

for CONFIG in ${DASHBOARD_CONFIGS}
do
  echo ${CONFIG}
  LOG=${BASEDIR}/../Logs/${CONFIG}
  ctest -S ${BASEDIR}/aaargh_${CONFIG}.cmake -VV 1>${LOG}.out 2>${LOG}.err
done
