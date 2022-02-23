#!/bin/bash
set +e

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

message1 "Building clang13 base image"
if ! build_squash \
  ornladios/adios2:ci-spack-el8-base \
  ornladios/adios2:ci-spack-el8-clang13-base \
  Dockerfile.ci-spack-el8-clang13-base
then
  echo "Error: Failed to build clang13 base image"
  exit 3
fi
