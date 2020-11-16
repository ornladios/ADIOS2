#!/bin/bash --login

if [ -n "${GITLAB_SITE}" ]
then
  export CI_SITE_NAME="${GITLAB_SITE}"
else
  export CI_SITE_NAME="GitLab CI"
fi

export CI_BUILD_NAME="${CI_COMMIT_BRANCH#github/}_${CI_JOB_NAME}"
export CI_SOURCE_DIR="${CI_PROJECT_DIR}"
export CI_ROOT_DIR="${CI_PROJECT_DIR}/.."
export CI_BIN_DIR="${CI_ROOT_DIR}/${CI_BUILD_NAME}"
export CI_COMMIT_REF=${CI_COMMIT_SHA}

STEP=$1
CTEST_SCRIPT=scripts/ci/cmake/ci-${CI_JOB_NAME}.cmake

# Update and Test steps enable an extra step
CTEST_STEP_ARGS=""
case ${STEP} in
  test) CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_end=ON" ;;
esac
CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_${STEP}=ON"

if [ -n "${CMAKE_ENV_MODULE}" ]
then
  module load ${CMAKE_ENV_MODULE}

  echo "**********module avail Begin************"
  module avail
  echo "**********module avail End**************"
fi

CTEST=ctest

echo "**********Env Begin**********"
env | sort
echo "**********Env End************"

echo "**********CTest Begin**********"
${CTEST} --version
echo ${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
RET=$?
echo "**********CTest End************"

exit ${RET}
