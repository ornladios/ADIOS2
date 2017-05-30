# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_NAME "Linux-EL7_Intel-17.0.3.191_NoMPI")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j72")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 18)
#set(dashboard_model Nightly)
set(dashboard_root_name "Builds/Intel-17.0.3.191_NoMPI")

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)
module(load intel)
module(load hdf5)

set(ENV{CC}  icc)
set(ENV{CXX} icpc)
set(ENV{FC}  ifort)

set(dashboard_cache "
ADIOS_USE_MPI:BOOL=OFF
ADIOS_USE_BZip2:BOOL=ON
ADIOS_USE_HDF5:BOOL=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/adios_common.cmake)
