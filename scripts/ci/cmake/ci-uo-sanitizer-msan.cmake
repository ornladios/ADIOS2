# Client maintainer: vicente.bolea@kitware.com

set(ENV{CC}  clang)
set(ENV{CXX} clang++)

set(dashboard_cache "
BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON

CMAKE_TOOLCHAIN_FILE:FILEPATH=/opt/msan/toolchain.cmake
ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF

HDF5_C_COMPILER_EXECUTABLE:FILEPATH=/opt/msan/bin/h5cc
HDF5_DIFF_EXECUTABLE:FILEPATH=/opt/msan/bin/h5diff
")

set(dashboard_track "Analysis")
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_MEMORYCHECK_TYPE "MemorySanitizer")

list(APPEND EXCLUDE_EXPRESSIONS
  "Install.*"
  "Staging.1x1DefSync.BP3"
  "Engine.DataMan.DataManEngineTest"
  "Unit.FileTransport.FailOnEOF.Serial"
  "Engine.BP.BPBufferSizeTest.SyncDeferredIdenticalUsage.*.Serial"
  )
list(JOIN EXCLUDE_EXPRESSIONS "|" TEST_EXCLUDE_STRING)
set(CTEST_MEMCHECK_ARGS EXCLUDE "${TEST_EXCLUDE_STRING}")

set(ADIOS_TEST_REPEAT 0)
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
