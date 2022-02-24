#!/bin/bash
set +e

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

for ver in 8 9 10 11
do
  message1 "Building gcc${ver}-serial image"
  if ! build_squash \
    ornladios/adios2:ci-spack-el8-gcc${ver}-base \
    ornladios/adios2:ci-spack-el8-gcc${ver}-serial \
    Dockerfile.ci-spack-el8-leaf \
    "--build-arg COMPILER=gcc${ver} --build-arg EXTRA_VARIANTS=~mpi"
  then
    echo "Error: Failed to build gcc${ver}-serial image"
    exit 3
  fi

  message1 "Building gcc${ver}-mpi image"
  if ! build_squash \
    ornladios/adios2:ci-spack-el8-gcc${ver}-base \
    ornladios/adios2:ci-spack-el8-gcc${ver}-mpi \
    Dockerfile.ci-spack-el8-leaf \
    "--build-arg COMPILER=gcc${ver} --build-arg EXTRA_VARIANTS=+mpi"
  then
    echo "Error: Failed to build gcc${ver}-mpi image"
    exit 3
  fi
done
