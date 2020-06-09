# Client maintainer: chuck.atkins@kitware.com

set(ENV{CC}  clang)
set(ENV{CXX} clang++)
set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -pthread")
set(ENV{CFLAGS}   "${ASAN_FLAGS}")
set(ENV{CXXFLAGS} "${ASAN_FLAGS}")
set(ENV{FFLAGS}   "${ASAN_FLAGS}")

set(dashboard_cache "
ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF
ADIOS2_USE_ZFP:STRING=ON
")

set(dashboard_track "Analysis")
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j4")
set(CTEST_MEMORYCHECK_TYPE "AddressSanitizer")

set(ADIOS_TEST_REPEAT 0)
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
