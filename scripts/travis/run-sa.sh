#!/usr/bin/env bash

if [ -z "${SOURCE_DIR}" ]
then
  echo "Error: SOURCE_DIR is empty or undefined"
  exit 1
fi

ANALYZER_INSTALL_DIR="${HOME}/clang+llvm-7.0.1-x86_64-linux-gnu-ubuntu-16.04"

# Make sure analyzer script find the right clang++
export PATH="${ANALYZER_INSTALL_DIR}/bin:${PATH}"
echo "PATH: ${PATH}"

clang_path=$(which clang++)
echo "clang path: ${clang_path}"
clang++ --version

export CCC_ANALYZER=$(find ${ANALYZER_INSTALL_DIR} | grep ccc-analyzer$)
export CXX_ANALYZER=$(find ${ANALYZER_INSTALL_DIR} | grep c++-analyzer$)
export WRAPPED_CC="${SOURCE_DIR}/scripts/travis/cl_analyze_wrap_gcc.sh"
export WRAPPED_CXX="${SOURCE_DIR}/scripts/travis/cl_analyze_wrap_gplusplus.sh"
export SUPPRESSOR="${SOURCE_DIR}/scripts/travis/custom_suppressor.sh"
export SUPPRESSIONS="${SOURCE_DIR}/scripts/travis/cl_analyze_suppressions.txt"

echo "CCC_ANALYZER=${CCC_ANALYZER}"
echo "CXX_ANALYZER=${CXX_ANALYZER}"

CUSTOM_BUILD_NAME="${BUILD_LABEL}_clang-analyzer"
CTEST_SCRIPT="${SOURCE_DIR}/scripts/travis/travis_clang-analyzer.cmake"

extra_checkers=(
  alpha.security.ArrayBoundV2
  alpha.security.MallocOverflow
  alpha.security.ReturnPtrRange
  alpha.security.taint.TaintPropagation
  alpha.unix.Chroot
  alpha.unix.PthreadLock
  alpha.unix.SimpleStream
  alpha.unix.Stream
  alpha.unix.cstring.BufferOverlap
  alpha.unix.cstring.NotNullTerminated
  alpha.unix.cstring.OutOfBounds
)

ANALYSES=""

for checker in "${extra_checkers[@]}"; do
  ANALYSES="${ANALYSES} -analyzer-checker=${checker}"
done

echo "Extra checkers environment variable: ${ANALYSES}"

# Environment variables used by analyzers
export CCC_ANALYZER_ANALYSIS=${ANALYSES}
# export CCC_ANALYZER_LOG=TRUE

steps=( update configure build )
for step in "${steps[@]}"; do
    echo "Running $step step"
    CL_ANALYZE=""
    if [ "$step" == "build" ]
    then
        CL_ANALYZE="TRUE"
    fi
    export DO_CL_ANALYZE=${CL_ANALYZE}
    ctest -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF -Ddashboard_do_${step}=TRUE -DCTEST_BUILD_NAME=${CUSTOM_BUILD_NAME}
done

exit 0
