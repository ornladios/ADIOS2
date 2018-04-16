# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j36")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 36)

set(CTEST_BUILD_NAME "Linux-EL7_Clang5_MSan")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)
module(load llvm5)

set(CTEST_MEMORYCHECK_TYPE "MemorySanitizer")
set(dashboard_do_memcheck ON)
set(dashboard_track "Analysis")

set(ENV{CC}  clang)
set(ENV{CXX} clang++)

set(dashboard_cache "
CMAKE_C_FLAGS=-fsanitize=memory -fno-omit-frame-pointer
CMAKE_CXX_FLAGS=-fsanitize=memory -fno-omit-frame-pointer

ADIOS2_USE_ADIOS1:STRING=OFF
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=OFF
ADIOS2_USE_HDF5:STRING=OFF
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF
ADIOS2_USE_ZFP:STRING=OFF
ADIOS2_USE_ZeroMQ:STRING=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)
