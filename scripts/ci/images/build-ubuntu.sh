#!/bin/bash

###
### To run with docker instead of podman, and push to your own repo rather than
### the main one, eg:
###
### IMAGE_BUILD_TOOL=docker IMAGE_TAG_PREFIX=ghcr.io/scottwittenburg/adios2 ./build-ubuntu.sh
###

set -ex

# Some overridable defaults
BUILD_TOOL="${IMAGE_BUILD_TOOL:-podman}"
TAG_PREFIX="${IMAGE_TAG_PREFIX:-ghcr.io/ornladios/adios2}"

# Build the base image
${BUILD_TOOL} build --progress=plain \
  --build-arg PATCH_VARIANT_XROOTD=ON \
  --rm -f ./Dockerfile.ci-spack-ubuntu22.04-base \
  -t "${TAG_PREFIX}:ci-spack-ubuntu22.04-base" \
  .

# Build the gcc9, gcc10, and gcc12 images
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}:ci-spack-ubuntu22.04-base" \
  --build-arg GCC_VERSION=12 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-gcc \
  -t "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc12" \
  .
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}:ci-spack-ubuntu22.04-base" \
  --build-arg GCC_VERSION=10 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-gcc \
  -t "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc10" \
  .
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}:ci-spack-ubuntu22.04-base" \
  --build-arg GCC_VERSION=9 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-gcc \
  -t "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc9" \
  .

# Build the clang11 and clang14 images
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}:ci-spack-ubuntu22.04-base" \
  --build-arg CLANG_VERSION=11 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-clang \
  -t "${TAG_PREFIX}:ci-spack-ubuntu22.04-clang11" \
  .
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}:ci-spack-ubuntu22.04-base" \
  --build-arg CLANG_VERSION=14 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-clang \
  -t "${TAG_PREFIX}:ci-spack-ubuntu22.04-clang14" \
  .

# Tag the ubuntu 22.04 gcc11 img
${BUILD_TOOL} tag "${TAG_PREFIX}:ci-spack-ubuntu22.04-base" "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc11"

# Push images to github container registry
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-base"
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc9"
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc10"
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-clang11"
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-clang14"
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc11"
${BUILD_TOOL} push "${TAG_PREFIX}:ci-spack-ubuntu22.04-gcc12"
