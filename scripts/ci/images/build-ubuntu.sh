#!/bin/bash

set -ex

# Build the base image
podman build --progress=plain \
  --build-arg EXTRA_VARIANTS="+blosc+ssc ^mgard@2023-01-10" \
  --build-arg PATCH_VARIANT_XROOTD=ON \
  --rm -f ./Dockerfile.ci-spack-ubuntu20.04-base \
  -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-base \
  .

# Build the gcc8, gcc9, and gcc10 images
podman build --rm --build-arg GCC_VERSION=8 -f ./Dockerfile.ci-spack-ubuntu20.04-gcc -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc8 .
podman build --rm --build-arg GCC_VERSION=10 -f ./Dockerfile.ci-spack-ubuntu20.04-gcc -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc10 .
podman build --rm --build-arg GCC_VERSION=9 -f ./Dockerfile.ci-spack-ubuntu20.04-gcc -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc9 .

# Build the clang6 and clang10 images
podman build --rm --build-arg CLANG_VERSION=6.0 -f ./Dockerfile.ci-spack-ubuntu20.04-clang -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang6 .
podman build --rm --build-arg CLANG_VERSION=10 -f ./Dockerfile.ci-spack-ubuntu20.04-clang -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang10 .

# Build the ubuntu 22.04 img
podman build \
  --progress=plain \
  --rm \
  --build-arg BASE_IMAGE="ecpe4s/ubuntu22.04-runner-amd64-gcc-11.4:2024.04.19" \
  --build-arg E4S_VERSION="24.05" \
  --build-arg EXTRA_VARIANTS="+blosc2" \
  -f ./Dockerfile.ci-spack-ubuntu20.04-base \
  -t ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-gcc11 \
  .

# Push images to github container registry
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-base
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc8
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc9
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc10
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang6
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang10
podman push ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-gcc11
