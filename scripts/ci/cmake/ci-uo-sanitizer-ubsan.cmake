# Client maintainer: vicente.bolea@kitware.com

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)
set(UBSAN_FLAGS "-fsanitize=undefined -fno-sanitize-recover=all -pthread")
set(ENV{CFLAGS}   "${UBSAN_FLAGS}")
set(ENV{CXXFLAGS} "${UBSAN_FLAGS}")
set(ENV{FFLAGS}   "${UBSAN_FLAGS}")
set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS "print_stacktrace=1")

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF

HDF5_C_COMPILER_EXECUTABLE:FILEPATH=/usr/bin/h5cc
HDF5_DIFF_EXECUTABLE:FILEPATH=/usr/bin/h5diff
")

set(dashboard_track "Analysis")
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_MEMORYCHECK_TYPE "UndefinedBehaviorSanitizer")

list(APPEND EXCLUDE_EXPRESSIONS
  "Install.*"
  "Unit.FileTransport.FailOnEOF.Serial"
  )
list(JOIN EXCLUDE_EXPRESSIONS "|" TEST_EXCLUDE_STRING)
set(CTEST_MEMCHECK_ARGS EXCLUDE "${TEST_EXCLUDE_STRING}")

set(ADIOS_TEST_REPEAT 0)
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
