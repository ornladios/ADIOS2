#!/usr/bin/env bash


export SOURCE_DIR=${TRAVIS_BUILD_DIR}
export BUILD_DIR=$(readlink -f ${SOURCE_DIR}/..)
export COMMIT_RANGE="${TRAVIS_COMMIT_RANGE/.../ }"

case ${BUILD_MATRIX_ENTRY} in
  format)
    echo "Running formatting tests"
    if ! ${SOURCE_DIR}/scripts/travis/run-format.sh; then
      exit 1;
    fi
    ;;
  *)
    echo "Error: BUILD_MATRIX_ENTRY is undefined or set to an unknown value"
    exit 1;
    ;;
esac
