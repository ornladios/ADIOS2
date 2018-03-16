# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j4")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 4)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_$ENV{CIRCLE_JOB}")
set(dashboard_track "Continuous Integration")

set(CTEST_GIT_COMMAND "/usr/bin/git")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{CIRCLE_WORKING_DIRECTORY}/source")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)

set(dashboard_cache "
ADIOS2_USE_ADIOS1:STRING=ON
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=ON
ADIOS2_USE_ZFP:STRING=ON
ADIOS2_USE_ZeroMQ:STRING=ON
ZFP_ROOT:PATH=/opt/zfp/install
ADIOS1_ROOT:PATH=/opt/adios1/1.12.0/gnu48

MEMORYCHECK_COMMAND:FILEPATH=/usr/bin/valgrind
MEMORYCHECK_COMMAND_OPTIONS:STRING=--leak-check=full --num-callers=50
")

# CTEST_MEMORYCHECK_SUPPRESSIONS_FILE:FILEPATH="/home/kitware/dashboards/supp.txt")

include(${CMAKE_CURRENT_LIST_DIR}/../dashboard/adios_common.cmake)
