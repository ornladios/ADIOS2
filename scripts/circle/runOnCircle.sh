#!/bin/bash

check_var() {
  if [ -z "$1" ]
  then
    echo "Error: The $1 environment variable is undefined"
    exit 1
  fi
}

check_var CIRCLE_WORKING_DIRECTORY
check_var CIRCLE_BRANCH
check_var CIRCLE_JOB

SOURCE_DIR=${CIRCLE_WORKING_DIRECTORY}/source
CTEST_SCRIPT="${SOURCE_DIR}/scripts/circle/circle_${CIRCLE_JOB}.cmake"

if [ ! -f "${CTEST_SCRIPT}" ]
then
  echo "Unable to find CTest script $(basename ${CTEST_SCRIPT})"
  exit 2
fi

case "$1" in
  configure|build|test)
    STEP=$1
    ;;
  *)
    echo "Usage: $0 (configure|build|test)"
    exit 3
    ;;
esac

/opt/cmake/3.6.0/bin/ctest -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF -Ddashboard_do_${STEP}=TRUE
