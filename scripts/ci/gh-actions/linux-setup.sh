#!/bin/bash

set -e

export CI_ROOT_DIR="${GITHUB_WORKSPACE}/.."
export CI_SOURCE_DIR="${GITHUB_WORKSPACE}"

SETUP_SCRIPT=${CI_SOURCE_DIR}/scripts/ci/setup/ci-${GH_YML_JOBNAME}.sh

if [ -x "${SETUP_SCRIPT}" ]
then
  echo "**********Setup Begin**********"
  echo ${SETUP_SCRIPT}
  ${SETUP_SCRIPT}
  echo "**********Setup End**********"
fi

exit 0
