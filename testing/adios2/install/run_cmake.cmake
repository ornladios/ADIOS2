#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

file(REMOVE_RECURSE "${ADIOS2_BINARY_DIR}/testing/adios2/install/cmake/${TEST_CASE}")
if(WIN32)
  set(ENV{PATH} "${ADIOS2_BINARY_DIR}/testing/adios2/install/install/${CMAKE_INSTALL_BINDIR};$ENV{PATH}")
endif()
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}"
  --build-and-test
  "${ADIOS2_SOURCE_DIR}/testing/adios2/install/${TEST_CASE}"
  "${ADIOS2_BINARY_DIR}/testing/adios2/install/cmake/${TEST_CASE}"
  --build-generator "${CMAKE_GENERATOR}"
  --build-makeprogram "${CMAKE_MAKE_PROGRAM}"
  --build-options
    "-Dadios2_DIR=${ADIOS2_BINARY_DIR}/testing/adios2/install/install/${CMAKE_INSTALL_CMAKEDIR}"
  --test-command "${CMAKE_CTEST_COMMAND}" -V
  RESULT_VARIABLE result
  )
if(result)
  message(FATAL_ERROR "Result of test was ${result}, should be 0")
endif()
