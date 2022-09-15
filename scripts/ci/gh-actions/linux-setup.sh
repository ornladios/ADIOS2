#!/bin/bash --login

set -ex

export CI_ROOT_DIR="${GITHUB_WORKSPACE}/.."
export CI_SOURCE_DIR="${GITHUB_WORKSPACE}"

declare -r local_scripts_dir="$(dirname -- $0)/config"

echo "**********Install dependencies Begin**********"

mkdir -p "${CI_ROOT_DIR}/.local/bin" || true

# Append bin to the workflow PATH file
echo "${CI_ROOT_DIR}/.local/bin" >> "$GITHUB_PATH"

find "$local_scripts_dir" -type f -name '*.sh' -perm /a=x -exec ./{} \;
find "$local_scripts_dir" -type f -name '*.cmake' -exec cmake --trace -VV -P {} \;

echo "**********Install dependencies End**********"

SETUP_SCRIPT=${CI_SOURCE_DIR}/scripts/ci/setup/ci-${GH_YML_JOBNAME}.sh

if [ -x "${SETUP_SCRIPT}" ]
then
  echo "**********Setup Begin**********"
  echo "${SETUP_SCRIPT}"
  "${SETUP_SCRIPT}"
  echo "**********Setup End**********"
fi

exit 0
