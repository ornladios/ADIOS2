#!/bin/bash

export CI_ROOT_DIR="${GITHUB_WORKSPACE//\\//}/.."
export CI_SOURCE_DIR="${GITHUB_WORKSPACE//\\//}"
export CI_DEP_DIR="${CI_ROOT_DIR}/dependencies"

# Install ninja, pkgconfig, and libfabric
case "$(uname -s)" in
  Linux)
    . $(dirname ${BASH_SOURCE[0]})/setup-linux.sh
    ;;
  Darwin)
    . $(dirname ${BASH_SOURCE[0]})/setup-macos.sh
    ;;
esac

export CMAKE_PREFIX_PATH=${CI_DEP_DIR}/install

