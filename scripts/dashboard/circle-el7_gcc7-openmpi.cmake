# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI EL7")
set(CTEST_BUILD_NAME "$ENV{CIRCLE_BRANCH}_gcc7-openmpi")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j4")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 4)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_gcc7-openmpi")

set(CTEST_SOURCE_DIRECTORY "$ENV{CIRCLE_WORKING_DIRECTORY}")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)
module(load gnu7)
module(load openmpi)
module(load phdf5)
module(load netcdf)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

include(${CMAKE_CURRENT_LIST_DIR}/adios_common.cmake)
