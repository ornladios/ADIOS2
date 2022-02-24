#!/bin/bash
set +e

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

message1 "Building intel base image"
if ! build_squash \
  ornladios/adios2:ci-spack-el8-base \
  ornladios/adios2:ci-spack-el8-intel-base \
  Dockerfile.ci-spack-el8-intel-base
then
  echo "Error: Failed to build intel base image"
  exit 3
fi

message1 "Building icc base image"
if ! build_squash \
  ornladios/adios2:ci-spack-el8-intel-base \
  ornladios/adios2:ci-spack-el8-icc-base \
  Dockerfile.ci-spack-el8-icc-base
then
  echo "Error: Failed to build icc base image"
  exit 3
fi

message1 "Building oneapi base image"
ADIOS2_CI_NO_SQUASH=1
if ! build_squash \
  ornladios/adios2:ci-spack-el8-intel-base \
  ornladios/adios2:ci-spack-el8-oneapi-base \
  Dockerfile.ci-spack-el8-oneapi-base
then
  echo "Error: Failed to build oneapi base image"
  exit 3
fi
