#!/bin/bash --login

if [ -n "${GITLAB_SITE}" ]
then
  export CI_SITE_NAME="${GITLAB_SITE}"
else
  export CI_SITE_NAME="GitLab CI"
fi

export CI_BUILD_NAME="${CI_COMMIT_BRANCH#github/}_${CI_JOB_NAME}"

CTEST_ARGS=""
if [ -n "${EXTERNAL_WORKDIR}" ]
then
  export CI_SOURCE_DIR="${EXTERNAL_WORKDIR}/source"
else
  export CI_SOURCE_DIR="${CI_PROJECT_DIR}"
fi
export CI_BIN_DIR="${CI_SOURCE_DIR}/../build"
export CI_COMMIT_REF=${CI_COMMIT_SHA}

CTEST_SCRIPT=scripts/ci/cmake/ci-${CI_BUILD_LABEL}.cmake
for STEP in "$@"
do
  if [[ "${STEP}" =~ "=" ]]
  then
    CTEST_ARGS="${CTEST_ARGS} -Ddashboard_do_${STEP}"
  else
    CTEST_ARGS="${CTEST_ARGS} -Ddashboard_do_${STEP}=ON"
  fi
done

CTEST=ctest

echo "**********Env Begin**********"
env | sort
echo "**********Env End************"

echo "**********CTest Begin**********"
${CTEST} --version
echo ${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_ARGS}
${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_ARGS}
RET=$?
echo "**********CTest End************"

exit ${RET}
