# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI")

set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j4")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_$ENV{CIRCLE_JOB}")
set(dashboard_track "Continuous Integration")

set(CTEST_GIT_COMMAND "/usr/bin/git")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{CIRCLE_WORKING_DIRECTORY}/source")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

include(${CMAKE_CURRENT_LIST_DIR}/EnvironmentModules.cmake)
module(purge)
module(load pgi)
module(load openmpi)

set(ENV{CC}  pgcc)
set(ENV{CXX} pgc++)
set(ENV{FC}  pgfortran)
#set(ENV{CFLAGS} -Werror)
set(ENV{CXXFLAGS} --brief_diagnostics)
#set(ENV{FFLAGS} "-warn errors")

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=OFF
ADIOS2_USE_MPI:STRING=ON
ADIOS2_USE_Python:STRING=OFF
ADIOS2_USE_ZFP:STRING=OFF
ADIOS2_USE_ZeroMQ:STRING=ON
#ZFP_ROOT:PATH=/opt/zfp/install

MPIEXEC_MAX_NUMPROCS:STRING=8
")

include(${CMAKE_CURRENT_LIST_DIR}/../dashboard/adios_common.cmake)
