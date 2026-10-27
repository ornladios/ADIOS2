#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -ex
DOCKER=${DOCKER:-podman}
DATE_TAG="${IMAGE_DATE:-$(date +%Y%m%d)}"
TAG_PREFIX="${IMAGE_TAG_PREFIX:-ghcr.io/ornladios/adios2}"

# Built the single el8 image
${DOCKER} build --rm -f Dockerfile.ci-el8-intel -t adios2:ci-el8-intel .

# Tag image
${DOCKER} tag adios2:ci-el8-intel "${TAG_PREFIX}/ci-el8-oneapi:${DATE_TAG}"
${DOCKER} tag adios2:ci-el8-intel "${TAG_PREFIX}/ci-el8-oneapi:latest"

# Push it
${DOCKER} push "${TAG_PREFIX}/ci-el8-oneapi:${DATE_TAG}"
${DOCKER} push "${TAG_PREFIX}/ci-el8-oneapi:latest"
