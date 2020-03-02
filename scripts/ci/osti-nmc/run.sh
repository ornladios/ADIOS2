#!/bin/bash

set -e

_CMAKE_DIR="$HOME/cmake/${CI_ENVIRONMENT}"
export PATH="$_CMAKE_DIR/bin:$PATH"
export ACLOCAL_PATH="$_CMAKE_DIR/share/aclocal:$ACLOCAL_PATH"

# Parse the branch name used by the PR
API_BASE="https://api.github.com/repos/ornladios/adios2"
REALBRANCH="${CI_COMMIT_REF_NAME}"

export CI_SITE_NAME="OSTI Gitlab (NMC)"

CLUSTER_NAME="$SLURM_CLUSTER_NAME"
if [ -z "$CLUSTER_NAME" ] ; then
    CLUSTER_NAME='unknown'
fi

export CI_BUILD_NAME="${REALBRANCH}_${CLUSTER_NAME}_${CI_ENVIRONMENT}"
export CI_COMMIT_REF=${CI_COMMIT_SHA}
export CI_SOURCE_DIR="${PWD}"
export CI_ROOT_DIR="$( dirname "${CI_SOURCE_DIR}" )"
export CI_BIN_DIR="${CI_ROOT_DIR}/build_${CI_PIPELINE_ID}"

STEP=$1
CTEST_SCRIPT=${CI_SOURCE_DIR}/scripts/ci/cmake/ci-osti-nmc.cmake

# Update and Test steps enable an extra step
CTEST_STEP_ARGS=""
case ${STEP} in
  update)
    CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_fresh=ON"
    CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_checkout=OFF"
    ;;
  test) CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_end=ON" ;;
esac

CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_${STEP}=ON"
CTEST=ctest

echo "**********Env Begin**********"
env | sort
echo "**********Env End************"

echo "**********CTest Begin**********"
which cmake
cmake --version
which ctest
${CTEST} --version
export FI_PROVIDER=tcp
echo ${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
RET=$?
echo "**********CTest End************"

exit ${RET}
