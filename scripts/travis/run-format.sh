#!/usr/bin/env bash

if [ -z "${SOURCE_DIR}" ]
then
  echo "Error: SOURCE_DIR is empty or undefined"
  exit 1
fi
if [ -z "${COMMIT_RANGE}" ]
then
  echo "Error: COMMIT_RANGE is empty or undefined"
  exit 1
fi

cd ${SOURCE_DIR}

# Check C and C++ code with clang-format
echo "Checking formatting for commit range: ${COMMIT_RANGE}"
DIFF="$(./scripts/developer/git/git-clang-format -x '^(thirdparty|cmake/upstream|bindings/Matlab)/' --diff ${COMMIT_RANGE})"
if [ -n "${DIFF}" ] && [ "${DIFF}" != "no modified files to format" ]
then
  echo "clang-format:"
  echo "  Code format checks failed."
  echo "  Please run clang-format (or git clang-format) on your changes"
  echo "  before committing."
  echo "  The following changes are suggested:"
  echo "${DIFF}"
  exit 1
fi

# Check python code with flake8
if ! ~/.local/bin/flake8 --config=flake8.cfg .
then
  exit 3
fi

exit 0
