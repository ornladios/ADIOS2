# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "aaargh.kitware.com")
set(CTEST_BUILD_NAME "Linux-EL7_Intel-17.0.3.191_IntelMPI")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j72")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 18)
#set(dashboard_model Nightly)
set(dashboard_root_name "Builds/Intel-17.0.3.191_IntelMPI")

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)
module(load intel)
module(load impi)
module(load phdf5)
module(load netcdf)
module(load adios)

set(ENV{CC}  icc)
set(ENV{CXX} icpc)
set(ENV{FC}  ifort)

set(dashboard_cache "
ADIOS_USE_MPI:BOOL=ON
ADIOS_USE_BZip2:BOOL=ON
ADIOS_USE_ADIOS1:BOOL=ON
ADIOS_USE_HDF5:BOOL=ON
ADIOS_USE_DataMan_ZeroMQ:BOOL=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/adios_common.cmake)
