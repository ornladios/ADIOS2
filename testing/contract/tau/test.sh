#!/bin/bash

set -x
set -e

source $(dirname $(readlink -f ${BASH_SOURCE}))/setup.sh

mkdir -p ${test_dir}
cd ${test_dir}

TAU=$(spack location -i tau)/bin/tau_exec

mpiexec -np 2 ${TAU} ${build_dir}/bin/adios2-variables-shapes-cpp

[ ! -f profile.0.0.0 ] || [ ! -s profile.0.0.0 ] && { echo "Error: file profile.0.0.0 not found or empty"; exit 1; }
[ ! -f profile.1.0.0 ] || [ ! -s profile.1.0.0 ] && { echo "Error: file profile.1.0.0 not found or empty"; exit 1; }

cat profile.0.0.0
cat profile.1.0.0
