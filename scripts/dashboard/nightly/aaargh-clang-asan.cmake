# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j36")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 36)

set(CTEST_BUILD_NAME "Linux-EL7_Clang5_ASan")
set(dashboard_model Nightly)
set(CTEST_DASHBOARD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/${CTEST_BUILD_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/../EnvironmentModules.cmake)
module(purge)

set(CTEST_MEMORYCHECK_TYPE "AddressSanitizer")
set(dashboard_do_memcheck ON)
set(dashboard_track "Analysis")

set(ENV{CC}  /opt/clang/latest/bin/clang)
set(ENV{CXX} /opt/clang/latest/bin/clang++)

set(dashboard_cache "
CMAKE_C_FLAGS=-fsanitize=address -fno-omit-frame-pointer
CMAKE_CXX_FLAGS=-fsanitize=address -fno-omit-frame-pointer

ADIOS2_USE_Fortran:STRING=OFF
")

include(${CMAKE_CURRENT_LIST_DIR}/../adios_common.cmake)
