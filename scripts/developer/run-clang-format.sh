#!/bin/bash

exec sudo docker run -itt --mount type=bind,source="$(pwd)",target=/root/adios2 \
  ghcr.io/ornladios/adios2:ci-formatting sh -c \
  "git config --global --add safe.directory /root/adios2 &&
   cd /root/adios2 &&
   ./scripts/ci/scripts/run-clang-format.sh"
