#!/bin/bash

set -ex

# export TAG_PREFIX="ghcr.io/ornladios/adios2"
export TAG_PREFIX="ghcr.io/scottwittenburg/adios2"

# Build the base image
docker build \
  --progress=plain \
  --rm \
  --build-arg BASE_IMAGE="rocm/dev-ubuntu-22.04" \
  -f ./Dockerfile.ci-spack-ubuntu22.04-rocm-base \
  -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-rocm-base \
  .

# Build the base image
docker build \
  --progress=plain \
  --rm \
  --build-arg BASE_IMAGE="${TAG_PREFIX}:ci-spack-ubuntu22.04-rocm-base" \
  --build-arg ENABLED_ENVS="serial" \
  --build-arg EXTERNAL_PACKAGES="hip rocprim" \
  --build-arg E4S_VERSION="25.06" \
  --build-arg EXTRA_VARIANTS="+kokkos+rocm amdgpu_target=gfx906 ^hip@7" \
  -f ./Dockerfile.ci-spack-ubuntu22.04-base \
  -t ${TAG_PREFIX}:ci-spack-ubuntu22.04-rocm \
  .

# Push images to github container registry
docker push ${TAG_PREFIX}:ci-spack-ubuntu22.04-rocm
