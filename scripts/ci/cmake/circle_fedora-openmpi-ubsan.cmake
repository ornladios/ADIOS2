# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j4")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_$ENV{CIRCLE_JOB}")
set(dashboard_track "Analysis")

set(CTEST_GIT_COMMAND "/usr/bin/git")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{CIRCLE_WORKING_DIRECTORY}/source")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)
set(ENV{CFLAGS} -fsanitize=undefined -fno-sanitize-recover=all -pthread)
set(ENV{CXXFLAGS} -fsanitize=undefined -fno-sanitize-recover=all -pthread)
set(ENV{FFLAGS} -fsanitize=undefined -fno-sanitize-recover=all -pthread)
set(ENV{UBSAN_OPTIONS} "print_stacktrace=1")

find_package(EnvModules REQUIRED)
env_module(load mpi)

set(ENV{CMAKE_PREFIX_PATH} "/opt/hdf5/1.10.4:$ENV{CMAKE_PREFIX_PATH}")

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=ON
ADIOS2_USE_Python:STRING=ON
ADIOS2_USE_ZFP:STRING=OFF
ADIOS2_USE_ZeroMQ:STRING=ON

MPIEXEC_MAX_NUMPROCS:STRING=4
")

include(${CMAKE_CURRENT_LIST_DIR}/../../dashboard/adios_common.cmake)
