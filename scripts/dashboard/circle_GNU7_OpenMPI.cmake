# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI PR Build")
set(CTEST_BUILD_NAME "$ENV{CIRCLE_BRANCH}_GNU7_OpenMPI")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)
set(CTEST_DASHBOARD_ROOT "$ENV{CTEST_DASHBOARD_ROOT}")

set(dashboard_model Experimental)
set(dashboard_root_name "Builds/GCC-7.1.0_OpenMPI")
set(dashboard_binary_name "GNU4_NoMPI")             # Keep build trees separate for each build
set(dashboard_git_branch $ENV{CIRCLE_BRANCH})       # Let dashboard know what branch CircleCI is testing

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)
module(load gnu7)
module(load openmpi)
module(load phdf5)
module(load netcdf)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS_USE_MPI:BOOL=ON
ADIOS_USE_BZip2:BOOL=ON
ADIOS_USE_HDF5:BOOL=ON
ADIOS_USE_DataMan_ZeroMQ:BOOL=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/adios_common.cmake)
