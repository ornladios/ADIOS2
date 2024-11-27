# Client maintainer: vicente.bolea@kitware.com

set(ENV{CC}  clang)
set(ENV{CXX} clang++)

#CMAKE_CXX_FLAGS=-O1 -fsanitize=memory -fsanitize-memory-track-origins -fno-optimize-sibling-calls
#CMAKE_C_FLAGS=-O1 -fsanitize=memory -fsanitize-memory-track-origins -fno-optimize-sibling-calls
#CMAKE_LINKER_FLAGS=-fsanitize=memory -fsanitize-memory-track-origins -fno-optimize-sibling-calls -fPIE

set(dashboard_cache "
CMAKE_TOOLCHAIN_FILE:FILEPATH=/opt/msan/toolchain.cmake
HDF5_C_COMPILER_EXECUTABLE:FILEPATH=/opt/msan/bin/h5cc
HDF5_DIFF_EXECUTABLE:FILEPATH=/opt/msan/bin/h5diff

BUILD_TESTING:BOOL=ON
ADIOS2_BUILD_EXAMPLES:BOOL=ON
ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF
")

set(dashboard_track "Analysis")
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_BUILD_FLAGS "-k0 -j8")
set(CTEST_MEMORYCHECK_TYPE "MemorySanitizer")

set(ADIOS_TEST_REPEAT 0)
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
