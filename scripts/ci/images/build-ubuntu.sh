#!/bin/bash

set -ex

# export TAG_PREFIX="ghcr.io/ornladios/adios2"
export TAG_PREFIX="ghcr.io/scottwittenburg/adios2"

# Build the base image
docker build --progress=plain \
  --build-arg PATCH_VARIANT_XROOTD=ON \
  --rm -f ./Dockerfile.ci-spack-ubuntu22.04-base \
  -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-base \
  .

# Build the gcc8, gcc9, and gcc10 images
docker build --rm --build-arg GCC_VERSION=12 -f ./Dockerfile.ci-spack-ubuntu22.04-gcc -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc12 .
docker build --rm --build-arg GCC_VERSION=10 -f ./Dockerfile.ci-spack-ubuntu22.04-gcc -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc10 .
docker build --rm --build-arg GCC_VERSION=9 -f ./Dockerfile.ci-spack-ubuntu22.04-gcc -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc9 .

# Build the clang6 and clang10 images
docker build --rm --build-arg CLANG_VERSION=11 -f ./Dockerfile.ci-spack-ubuntu22.04-clang -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-clang11 .
docker build --rm --build-arg CLANG_VERSION=14 -f ./Dockerfile.ci-spack-ubuntu22.04-clang -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-clang14 .

# Build the ubuntu 22.04 gcc11 img
docker tag ${TAG_PREFIX}:ci-spack-ubuntu22.04-base ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc11

# Push images to github container registry
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-base
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc9
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc10
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-clang11
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-clang14
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc11
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc12
