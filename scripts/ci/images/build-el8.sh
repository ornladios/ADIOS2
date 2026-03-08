#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

set -ex
DOCKER=${DOCKER:-podman}

# Built the single el8 image
${DOCKER} build --rm -f Dockerfile.ci-el8-intel -t adios2:ci-el8-intel .

# Tag images
${DOCKER} tag adios2:ci-el8-intel ghcr.io/ornladios/adios2:ci-el8-oneapi
${DOCKER} tag adios2:ci-el8-intel ghcr.io/ornladios/adios2:ci-el8-icc

# Push them
${DOCKER} push ghcr.io/ornladios/adios2:ci-el8-oneapi
${DOCKER} push ghcr.io/ornladios/adios2:ci-el8-icc
