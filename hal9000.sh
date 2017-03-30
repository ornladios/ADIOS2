#!/bin/bash

BASEDIR=$(readlink -f $(dirname ${BASH_SOURCE}))
cd ${BASEDIR}

echo "Updating scripts..."
git pull --ff-only
cd ${HOME}
mkdir -p ${BASEDIR}/../Logs

module load mpi/mpich

LOGBASE=${BASEDIR}/../Logs/hal9000
ctest -S ${BASEDIR}/hal9000.cmake -VV 1>>${LOGBASE}.out 2>${LOGBASE}.err

