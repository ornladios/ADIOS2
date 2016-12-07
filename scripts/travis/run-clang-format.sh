#!/usr/bin/env bash

if [ -z "${TRAVIS_PULL_REQUEST_BRANCH}" ]
then
  echo "This is only designed to run on Pull Requests"
  exit 1
else
  COMMIT_RANGE="${TRAVIS_COMMIT_RANGE/.../ }"
fi

# Run clang-format
DIFF="$(${TRAVIS_BUILD_DIR}/scripts/developer/git/git-clang-format --diff ${COMMIT_RANGE})"
if [ "${DIFF}" != "no modified files to format" ]
then
  echo "clang-format:"
  echo "  Code format checks failed."
  echo "  Please run clang-format (or git clang-format) on your changes"
  echo "  before committing."
  echo "  The following changes are suggested:"
  echo "${DIFF}"
  exit 1
fi

exit 0
