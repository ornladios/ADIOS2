#!/usr/bin/env bash

export SOURCE_DIR=${TRAVIS_BUILD_DIR}
export BUILD_DIR="${SOURCE_DIR}/.."
export COMMIT_RANGE="${TRAVIS_COMMIT_RANGE/.../ }"

case ${BUILD_MATRIX_ENTRY} in
  format)
    echo "Running formatting tests"
    if ! ${SOURCE_DIR}/scripts/travis/run-format.sh; then
      exit 1;
    fi
    ;;
  analyze)
    echo "Running static analysis (clang-analyzer)"
    if ! ${SOURCE_DIR}/scripts/travis/run-sa.sh; then
      exit 1;
    fi
    ;;
  check)
    echo "Running static analysis (cppcheck)"
    if ! ${SOURCE_DIR}/scripts/travis/run-cppcheck.sh; then
      exit 1;
    fi
    ;;
  *)
    case ${TRAVIS_OS_NAME} in
      linux)
        TRAVIS_GENERATOR="Unix Makefiles"
        TRAVIS_BUILD_NAME="ubnt-trusty"
        ;;
      osx)
        TRAVIS_GENERATOR="Xcode"
        TRAVIS_BUILD_NAME="osx-${TRAVIS_OSX_IMAGE}"
        ;;
      *)
        echo "Error: BUILD_MATRIX_ENTRY is undefined or set to an unknown value"
        exit 1;
        ;;
    esac
    TRAVIS_BUILD_NAME="${TRAVIS_BUILD_NAME}-${CC}"
    if [ -n "${MPI}" ]
    then
      TRAVIS_BUILD_NAME="${TRAVIS_BUILD_NAME}-${MPI}"
    fi
    export TRAVIS_BUILD_NAME TRAVIS_GENERATOR
    echo "Running build"
    ${SOURCE_DIR}/scripts/travis/run-build.sh
    ;;
esac
