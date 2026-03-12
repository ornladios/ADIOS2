# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


unset(ENV{DESTDIR})
file(REMOVE_RECURSE "${ADIOS2_BINARY_DIR}/testing/install/install")
execute_process(COMMAND "${CMAKE_COMMAND}"
  "-DCMAKE_INSTALL_PREFIX=${ADIOS2_BINARY_DIR}/testing/install/install"
  "-DBUILD_TYPE=${BUILD_TYPE}"
  -P "${ADIOS2_BINARY_DIR}/cmake_install.cmake"
  RESULT_VARIABLE result
  )
if(result)
  message(FATAL_ERROR "Result of installation was ${result}, should be 0")
endif()
