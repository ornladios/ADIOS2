#!/bin/bash
BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

message1 "Building nvhpc222 serial image"
if ! build_squash \
  ornladios/adios2:ci-spack-el8-gcc8-serial \
  ornladios/adios2:ci-spack-el8-nvhpc222-serial \
  Dockerfile.ci-spack-el8-nvhpc222-leaf \
  "--build-arg PARALLEL=serial"
then
  echo "Error: Failed to build nvhpc222 serial image"
  exit 3
fi

message1 "Building nvhpc222 mpi image"
if ! build_squash \
  ornladios/adios2:ci-spack-el8-gcc8-mpi \
  ornladios/adios2:ci-spack-el8-nvhpc222-mpi \
  Dockerfile.ci-spack-el8-nvhpc222-leaf \
  "--build-arg PARALLEL=mpi"
then
  echo "Error: Failed to build nvhpc222 mpi image"
  exit 3
fi
