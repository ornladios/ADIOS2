#!/bin/bash

set -ex

function docker_build() {
  # shellcheck disable=SC2068
  sudo docker build --progress=plain --rm --push $@ .
}

# Build the base image
docker_build \
  --build-arg EXTRA_VARIANTS="+blosc+ssc" \
  --build-arg PATCH_VARIANT_XROOTD=ON \
  --file ./Dockerfile.ci-spack-ubuntu20.04-base \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-base-20241030

# Build the gcc8, gcc9, and gcc10 images
docker_build \
  --build-arg GCC_VERSION=8 \
  --file ./Dockerfile.ci-spack-ubuntu20.04-gcc \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc8-20241030

docker_build \
  --build-arg GCC_VERSION=10 \
  --file ./Dockerfile.ci-spack-ubuntu20.04-gcc \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc10-20241030

docker_build \
  --build-arg GCC_VERSION=9 \
  --file ./Dockerfile.ci-spack-ubuntu20.04-gcc \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc9-20241030

docker_build \
  --build-arg CLANG_VERSION=6.0 \
  --file ./Dockerfile.ci-spack-ubuntu20.04-clang \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang6-20241030

docker_build \
  --build-arg CLANG_VERSION=10 \
  --file ./Dockerfile.ci-spack-ubuntu20.04-clang \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang10-20241030


# Build the ubuntu 22.04 img
docker_build \
  --build-arg BASE_IMAGE="ecpe4s/ubuntu22.04-runner-amd64-gcc-11.4:2024.04.19" \
  --build-arg E4S_VERSION="24.05" \
  --build-arg EXTRA_VARIANTS="+blosc2" \
  --file ./Dockerfile.ci-spack-ubuntu20.04-base \
  --tag ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-gcc11 \
