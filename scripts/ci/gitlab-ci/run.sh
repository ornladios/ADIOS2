#!/bin/bash --login
# shellcheck disable=SC1091
set -ex

die() { echo "error: $1"; exit "$2"; }

enable_submission=ON
while getopts "hs" opt; do
  case $opt in
    h)
      echo "Usage: $0 [-s] [update|configure|build|test|memcheck|submit]"
      echo "OPTIONS:"
      echo "    -s skip submit to cdash"
      ;;
    s)
      enable_submission=OFF
      ;;
    *)
      die "Invalid option: -$OPTARG" 1
      ;;
  esac
done
shift $((OPTIND-1))
readonly STEP=$1
[ -z "$STEP" ] && die "no argument given: $*" 2
shift

source scripts/ci/gitlab-ci/setup-vars.sh

# Load any additional setup scripts
if [ -f "scripts/ci/setup-run/ci-${CI_JOB_NAME}.sh" ]
then
  # shellcheck source=/dev/null
  source "scripts/ci/setup-run/ci-${CI_JOB_NAME}.sh"
fi

readonly CTEST_SCRIPT=scripts/ci/cmake/ci-${CI_JOB_NAME}.cmake
[ ! -f "$CTEST_SCRIPT" ] && die "variable files does not not exits: $CTEST_SCRIPT" 3

declare -a CTEST_STEP_ARGS=("-Ddashboard_full=OFF" "-Ddashboard_do_${STEP}=ON" "-Ddashboard_do_submit=${enable_submission}")
case ${STEP} in
  update) CTEST_STEP_ARGS+=("${CI_UPDATE_ARGS}") ;;
  submit) CTEST_STEP_ARGS+=("-Ddashboard_full=ON" "-Ddashboard_do_submit_only=ON -Ddashboard_do_submit=ON") ;;
esac

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
