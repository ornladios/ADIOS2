#!/bin/bash

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

message1 "Building ci-spack-el8 base image"
if ! build_squash \
  almalinux:8 \
  ornladios/adios2:ci-spack-el8-base \
  Dockerfile.ci-spack-el8-base
then
  echo "Error: Failed to build ci-spack-el8 base image"
  exit 3
fi
