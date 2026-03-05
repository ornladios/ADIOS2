#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

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
DATE_TAG="${IMAGE_DATE:-$(date +%Y%m%d)}"

# Build the base image
${BUILD_TOOL} build --progress=plain \
  --build-arg PATCH_VARIANT_XROOTD=ON \
  --build-arg EXTRA_SPECS="sz3" \
  --rm -f ./Dockerfile.ci-spack-ubuntu22.04-base \
  -t "${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest" \
  .

# Build the gcc10, gcc12, and gcc14 images
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest" \
  --build-arg GCC_VERSION=14 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-gcc \
  -t "${TAG_PREFIX}/ci-spack-ubuntu22.04-gcc14:${DATE_TAG}" \
  .
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest" \
  --build-arg GCC_VERSION=12 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-gcc \
  -t "${TAG_PREFIX}/ci-spack-ubuntu22.04-gcc12:${DATE_TAG}" \
  .
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest" \
  --build-arg GCC_VERSION=10 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-gcc \
  -t "${TAG_PREFIX}/ci-spack-ubuntu22.04-gcc10:${DATE_TAG}" \
  .

# Build the clang11 and clang14 images
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest" \
  --build-arg CLANG_VERSION=11 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-clang \
  -t "${TAG_PREFIX}/ci-spack-ubuntu22.04-clang11:${DATE_TAG}" \
  .
${BUILD_TOOL} build --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest" \
  --build-arg CLANG_VERSION=14 \
  -f ./Dockerfile.ci-spack-ubuntu22.04-clang \
  -t "${TAG_PREFIX}/ci-spack-ubuntu22.04-clang14:${DATE_TAG}" \
  .

# Push images to github container registry
${BUILD_TOOL} push "${TAG_PREFIX}/ci-spack-ubuntu22.04-base:latest"
for img in gcc10 gcc12 gcc14 clang11 clang14; do
  ${BUILD_TOOL} tag "${TAG_PREFIX}/ci-spack-ubuntu22.04-${img}:${DATE_TAG}" \
    "${TAG_PREFIX}/ci-spack-ubuntu22.04-${img}:latest"
  ${BUILD_TOOL} push "${TAG_PREFIX}/ci-spack-ubuntu22.04-${img}:${DATE_TAG}"
  ${BUILD_TOOL} push "${TAG_PREFIX}/ci-spack-ubuntu22.04-${img}:latest"
done
