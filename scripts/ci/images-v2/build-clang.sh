#!/bin/bash
BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

if ! build_leafs clang13 clang
then
  exit 1
fi
