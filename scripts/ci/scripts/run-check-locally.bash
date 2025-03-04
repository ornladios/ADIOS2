#!/bin/bash

set -x

readonly DOCKER_IMG=ghcr.io/ornladios/adios2:ci-formatting
readonly PODMAN_CMD=(podman run -v .:/data -w /data -it "${DOCKER_IMG}")

read -r -d '' HELP_MSG << EOS
  run-check-locally.bash <action>
    -c    clang-format
    -p    pylint
    -k    shellcheck
EOS

if (( "$#" == 0 ))
then
  echo "error: action not given"
  echo "${HELP_MSG}"
  exit 1
fi

while getopts "cps" o
do
  case "${o}" in
    c)
      "${PODMAN_CMD[@]}" "./scripts/ci/scripts/run-clang-format.sh"
      ;;
    p)
      "${PODMAN_CMD[@]}" "./scripts/ci/scripts/run-pylint.sh"
      ;;
    s)
      "${PODMAN_CMD[@]}" "./scripts/ci/scripts/run-shellcheck.sh"
      ;;
    *)
      echo "error: action not recognized"
      echo "${HELP_MSG}"
      exit 2
      ;;
  esac
done
