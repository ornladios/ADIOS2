#!/usr/bin/env bash

echo "---------- Begin ENV ----------"
env | sort
echo "----------  End ENV  ----------"

# If a source dir is given, then change to it.  Otherwise assume we're already
# in it
if [ -n "${SOURCE_DIR}" ]
then
  cd ${SOURCE_DIR}
fi

# Check C and C++ code with clang-format
find source plugins testing examples bindings -regextype posix-extended -iregex '.*\.(h|c|cpp|tcc|cu)'  | xargs clang-format -i
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
