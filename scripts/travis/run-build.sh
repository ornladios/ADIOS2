#!/usr/bin/env bash

if [ -z "${SOURCE_DIR}" ]
then
  echo "Error: SOURCE_DIR is empty or undefined"
  exit 1
fi

export FC=gfortran
CTEST_SCRIPT="${SOURCE_DIR}/scripts/travis/travis_build.cmake"
ctest -VV -S ${CTEST_SCRIPT} \
  -Ddashboard_full=OFF \
  -Ddashboard_do_submit=ON \
  -Ddashboard_do_update=ON \
  -Ddashboard_do_configure=ON \
  -Ddashboard_do_build=ON \
  -Ddashboard_do_test=ON
