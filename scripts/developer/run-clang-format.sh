#!/bin/bash

CONTAINER_DRIVER=${CONTAINER_DRIVER:-docker}

"${CONTAINER_DRIVER[@]}" run -itt --mount type=bind,source="$(pwd)",target=/root/adios2 \
  ghcr.io/ornladios/adios2:ci-formatting sh -c \
  "git config --global --add safe.directory /root/adios2 &&
   cd /root/adios2 &&
   ./scripts/ci/scripts/run-clang-format.sh"

if [ "${CONTAINER_DRIVER[0]}" != "podman" ]
then
  git status --porcelain | awk '{print $2}' | xargs sudo chown "$USER:$(id -g)"
fi
