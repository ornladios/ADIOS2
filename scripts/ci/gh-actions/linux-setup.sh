#!/bin/bash --login

set -ex

export CI_ROOT_DIR="${GITHUB_WORKSPACE}/.."
export CI_SOURCE_DIR="${GITHUB_WORKSPACE}"

echo "**********Install dependencies Begin**********"
[ -d ${CI_ROOT_DIR}/.local/bin ] || mkdir -p ${CI_ROOT_DIR}/.local/bin

find ${CI_SOURCE_DIR}/gha/scripts/ci/gh-actions/config/ -type f -iname '*.sh ' -perm /a=x -exec ./{} \;
find ${CI_SOURCE_DIR}/gha/scripts/ci/gh-actions/config/ -type f -iname '*.cmake' -exec cmake --trace -VV -P {} \;
echo "**********Install dependencies End**********"

SETUP_SCRIPT=${CI_SOURCE_DIR}/scripts/ci/setup/ci-${GH_YML_JOBNAME}.sh

if [ -x "${SETUP_SCRIPT}" ]
then
  echo "**********Setup Begin**********"
  echo ${SETUP_SCRIPT}
  ${SETUP_SCRIPT}
  echo "**********Setup End**********"
fi

exit 0
