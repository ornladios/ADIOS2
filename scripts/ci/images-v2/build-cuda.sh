#!/bin/bash
set +e

BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

message1 "Building ci-spack-el8 base image"
if ! build_squash \
    nvidia/cuda:11.7.0-devel-rockylinux8 \
    ornladios/adios2:ci-spack-el8-cuda-base \
    Dockerfile.ci-spack-el8-base \
    "--build-arg=BASE_IMAGE=nvidia/cuda:11.7.0-devel-rockylinux8"
then
  echo "Error: Failed to build ci-spack-el8 base image"
  exit 3
fi

message1 "Building ci-spack-el8 gcc8 base image"
if ! build_squash \
    ornladios/adios2:ci-spack-el8-cuda-base \
    ornladios/adios2:ci-spack-el8-cuda-gcc8-base \
    Dockerfile.ci-spack-el8-gcc8-base \
    "--build-arg=BASE_IMAGE=ornladios/adios2:ci-spack-el8-cuda-base"
then
  echo "Error: Failed to build ci-spack-el8 leaf image"
  exit 3
fi

message1 "Building ci-spack-el8 leaf image"
build_conf=cuda-gcc8
if ! build_squash \
    ornladios/adios2:ci-spack-el8-cuda-gcc8-base \
    ornladios/adios2:ci-spack-el8-cuda-serial \
    Dockerfile.ci-spack-el8-leaf \
    "--build-arg=COMPILER_IMG_BASE=${build_conf} --build-arg=COMPILER_SPACK_ID=gcc --build-arg=CUDA_VARIANT=+cuda"
then
  echo "Error: Failed to build ci-spack-el8 leaf image"
  exit 3
fi
