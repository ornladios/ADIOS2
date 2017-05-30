# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_NAME "Linux-EL7_GCC-4.8.5_NoMPI")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j72")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 18)
#set(dashboard_model Nightly)
set(dashboard_root_name "Builds/GCC-4.8.5_NoMPI")

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS_USE_MPI:BOOL=OFF
ADIOS_USE_BZip2:BOOL=ON
ADIOS_USE_DataMan_ZeroMQ:BOOL=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/adios_common.cmake)
