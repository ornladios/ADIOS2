#!/bin/bash
set -ex

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

message1 "Building ci-spack-el8 leaf image"
build_conf=gcc8
if ! build_squash \
    ornladios/adios2:ci-spack-el8-gcc8-base \
    ornladios/adios2:ci-spack-el8-cuda-serial \
    Dockerfile.ci-spack-el8-leaf \
    "--build-arg=COMPILER_IMG_BASE=${build_conf}
     --build-arg=COMPILER_SPACK_ID=gcc
     --build-arg=DEVICE=CUDA
     --build-arg=MPI_VARIANT=~mpi"
then
  echo "Error: Failed to build ci-spack-el8 leaf image"
  exit 3
fi
