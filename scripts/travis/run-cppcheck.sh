#!/usr/bin/env bash

if [ -z "${SOURCE_DIR}" ]
then
  echo "Error: SOURCE_DIR is empty or undefined"
  exit 1
fi

CPPCHECK_EXE=/opt/cppcheck/install/bin/cppcheck

export WRAPPED_CC="${SOURCE_DIR}/scripts/travis/cppcheck_wrap_gcc.sh"
export WRAPPED_CXX="${SOURCE_DIR}/scripts/travis/cppcheck_wrap_gplusplus.sh"
export SUPPRESSOR="${SOURCE_DIR}/scripts/travis/custom_suppressor.sh"
export SUPPRESSIONS="${SOURCE_DIR}/scripts/travis/cppcheck_suppressions.txt"
export CPPCHECK_EXE=${CPPCHECK_EXE}

cd ${BUILD_DIR}

CUSTOM_BUILD_NAME="${BUILD_LABEL}_cppcheck"
CTEST_SCRIPT="${SOURCE_DIR}/scripts/travis/travis_cppcheck.cmake"

${CPPCHECK_EXE} --version
${CPPCHECK_EXE} --help

steps=( update configure build )
for step in "${steps[@]}"; do
    echo "Running $step step"
    CPPCHECK=""
    if [ "$step" == "build" ]
    then
        CPPCHECK="TRUE"
    fi
    export DOCPPCHECK=${CPPCHECK}
    ctest -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF -Ddashboard_do_${step}=TRUE -DCTEST_BUILD_NAME=${CUSTOM_BUILD_NAME}
done

exit 0
