#!/bin/bash
BASE_DIR=$(dirname $(readlink -f ${BASH_SOURCE}))
source "${BASE_DIR}/build-functions.sh"

if ! build_leafs icc intel
then
  exit 1
fi

if ! build_leafs oneapi oneapi
then
  exit 2
fi
