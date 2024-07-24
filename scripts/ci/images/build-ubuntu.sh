#!/bin/bash

set -ex

# Build the base image
docker build --progress=plain --build-arg EXTRA_VARIANTS="+blosc+ssc ^mgard@2023-01-10" --rm -f ./Dockerfile.ci-spack-ubuntu20.04-base -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-base .

# Build the gcc8, gcc9, and gcc10 images
docker build --rm --build-arg GCC_VERSION=8 -f ./Dockerfile.ci-spack-ubuntu20.04-gcc -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc8 .
docker build --rm --build-arg GCC_VERSION=10 -f ./Dockerfile.ci-spack-ubuntu20.04-gcc -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc10 .
docker build --rm --build-arg GCC_VERSION=9 -f ./Dockerfile.ci-spack-ubuntu20.04-gcc -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc9 .

# Build the clang6 and clang10 images
docker build --rm --build-arg CLANG_VERSION=6.0 -f ./Dockerfile.ci-spack-ubuntu20.04-clang -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang6 .
docker build --rm --build-arg CLANG_VERSION=10 -f ./Dockerfile.ci-spack-ubuntu20.04-clang -t ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang10 .

# Build the ubuntu 22.04 img
docker build \
  --progress=plain \
  --rm \
  --build-arg BASE_IMAGE="ecpe4s/ubuntu22.04-runner-amd64-gcc-11.4:2024.04.19" \
  --build-arg E4S_VERSION="24.05" \
  --build-arg EXTRA_VARIANTS="+blosc2" \
  -f ./Dockerfile.ci-spack-ubuntu20.04-base \
  -t ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-gcc11 \
  .

# Push images to github container registry
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-base
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc8
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc9
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-gcc10
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang6
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu20.04-clang10
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-gcc11
