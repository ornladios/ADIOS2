#!/bin/bash --login
set -e

export CI_BUILD_NAME="${CI_COMMIT_BRANCH#github/}_${CI_JOB_NAME}"
export CI_COMMIT_REF=${CI_COMMIT_SHA}
export CI_ROOT_DIR="${CI_PROJECT_DIR}/.."
export CI_SITE_NAME="${GITLAB_SITE}"
export CI_SOURCE_DIR="${CI_PROJECT_DIR}"

export CI_BIN_DIR="${CI_ROOT_DIR}/${CI_BUILD_NAME}"

readonly CTEST_SCRIPT=scripts/ci/cmake-v2/ci-${CI_JOB_NAME}.cmake
if [ ! -f "$CTEST_SCRIPT" ]
then
  echo "[E] Variable files does not exits: $CTEST_SCRIPT"
  exit 1
fi

readonly STEP=$1
if [ -z "$STEP" ]
then
  echo "[E] No argument given: $*"
  exit 2
fi

# In OLCF Gitlab our PRs branches tip commit is not the head commit of the PR,
# it is instead the so called merged_commit_sha as described in the GitHub Rest
# API for pull requests. We need to report to the CDASH the original commit
# thus, we set it here using the CTEST_UPDATE_VERSION_OVERRIDE CMake variable
if [[ ${CI_COMMIT_BRANCH} =~ ^pr[0-9]+_.*$ ]]
then
  # Original commit it is always its 2nd parent
  original_sha=$(git rev-parse "${CI_COMMIT_REF}^2")
  readonly UPDATE_ARGS="-DCTEST_UPDATE_VERSION_OVERRIDE=${original_sha}"
fi

declare -a CTEST_STEP_ARGS=("-Ddashboard_full=OFF")
case ${STEP} in
  update) CTEST_STEP_ARGS+=("${UPDATE_ARGS}") ;;
  build)  CTEST_STEP_ARGS+=("-Ddashboard_do_submit=OFF") ;;
  test)   CTEST_STEP_ARGS+=("-Ddashboard_do_submit=OFF") ;;
  submit) CTEST_STEP_ARGS+=("-Ddashboard_do_submit_only=ON" "-Ddashboard_do_build=ON" "-Ddashboard_do_test=ON") ;;
esac
CTEST_STEP_ARGS+=("-Ddashboard_do_${STEP}=ON")

echo "**********CTest Begin**********"
echo "ctest -VV -S ${CTEST_SCRIPT} ${CTEST_STEP_ARGS[*]}"
ctest -VV -S "${CTEST_SCRIPT}" "${CTEST_STEP_ARGS[@]}"
RET=$?
echo "**********CTest End************"

# EC: 0-127 this script errors, 128-INF ctest errors
if [ $RET -ne 0 ]
then
  (( RET += 127 ))
fi
exit $RET
