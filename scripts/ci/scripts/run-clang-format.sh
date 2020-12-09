#!/usr/bin/env bash

echo "---------- Begin ENV ----------"
env | sort
echo "----------  End ENV  ----------"

if [ -z "${SOURCE_DIR}" ]
then
  echo "Error: SOURCE_DIR is empty or undefined"
  exit 1
fi

cd ${SOURCE_DIR}

# Check C and C++ code with clang-format
find source testing examples bindings -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.tcc' | xargs clang-format -i
DIFF="$(git diff)"
if [ -n "${DIFF}" ]
then
  echo "clang-format:"
  echo "  Code format checks failed."
  echo "  Please run clang-format v7.1.0 on your changes before committing."
  echo "  The following changes are suggested:"
  echo "${DIFF}"
  exit 1
fi

exit 0
