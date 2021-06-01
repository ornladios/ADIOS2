#!/bin/bash

set -x
set -e

source $(dirname $(readlink -f ${BASH_SOURCE}))/setup.sh

mkdir -p ${test_dir}
cd ${test_dir}

TAU=$(spack location -i tau)/bin/tau_exec

mpiexec -np 2 ${TAU} ${build_dir}/bin/adios2-variables-shapes-cpp

cat profile.0.0.0
cat profile.1.0.0
