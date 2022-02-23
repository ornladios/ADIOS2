#!/bin/bash
set +e

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

for ver in 8 9 10 11
do
  message1 "Building gcc${ver} base image"
  if ! build_squash \
    ornladios/adios2:ci-spack-el8-base \
    ornladios/adios2:ci-spack-el8-gcc${ver}-base \
    Dockerfile.ci-spack-el8-gcc${ver}-base
  then
    echo "Error: Failed to build gcc${ver} base image"
    exit 3
  fi
done
