#!/bin/bash

set -ex

# Build the base image
docker build \
  --progress=plain \
  --rm \
  --build-arg BASE_IMAGE="rocm/dev-ubuntu-22.04" \
  -f ./Dockerfile.ci-spack-ubuntu22.04-rocm-base \
  -t ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-rocm-base \
  .

# Build the base image
docker build \
  --progress=plain \
  --rm \
  --build-arg BASE_IMAGE="ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-rocm-base" \
  --build-arg ENABLED_ENVS="serial" \
  --build-arg EXTERNAL_PACKAGES="hip rocprim" \
  --build-arg E4S_VERSION="24.05" \
  --build-arg EXTRA_VARIANTS="+blosc2+kokkos+rocm amdgpu_target=gfx906 ^hip@6" \
  -f ./Dockerfile.ci-spack-ubuntu20.04-base \
  -t ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-rocm \
  .

# Push images to github container registry
docker push ghcr.io/ornladios/adios2:ci-spack-ubuntu22.04-rocm
