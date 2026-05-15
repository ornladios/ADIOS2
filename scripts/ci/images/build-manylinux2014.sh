#!/bin/bash
# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -ex

# Some overridable defaults
BUILD_TOOL="${IMAGE_BUILD_TOOL:-podman}"
TAG_PREFIX="${IMAGE_TAG_PREFIX:-ghcr.io/ornladios/adios2}"
DATE_TAG="${IMAGE_DATE:-$(date +%Y%m%d)}"

# Build the gcc12 image
${BUILD_TOOL} build --rm \
  --build-arg GCC_VERSION=11 \
  -f ./Dockerfile.ci-manylinux2014-gcc \
  -t "${TAG_PREFIX}/ci-manylinux2014-gcc11:${DATE_TAG}" \
  .

# Push image to github container registry
${BUILD_TOOL} tag "${TAG_PREFIX}/ci-manylinux2014-gcc11:${DATE_TAG}" \
  "${TAG_PREFIX}/ci-manylinux2014-gcc11:latest"
${BUILD_TOOL} push "${TAG_PREFIX}/ci-manylinux2014-gcc11:${DATE_TAG}"
${BUILD_TOOL} push "${TAG_PREFIX}/ci-manylinux2014-gcc11:latest"
