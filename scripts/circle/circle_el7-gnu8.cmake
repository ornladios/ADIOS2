# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI")

set(CTEST_BUILD_CONFIGURATION Debug)
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
module(load gnu8)
module(load py3-numpy)
module(load hdf5)

set(ENV{CC}  gcc)
set(ENV{CXX} g++)
set(ENV{FC}  gfortran)
set(ENV{CFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{CXXFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
set(ENV{FFLAGS} "-Werror -Wno-error=builtin-declaration-mismatch")
find_program(PYTHON_EXECUTABLE python3.4)

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_Fortran:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=ON
#ADIOS2_USE_ZFP:STRING=ON
ADIOS2_USE_ZeroMQ:STRING=ON

PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
")

include(${CMAKE_CURRENT_LIST_DIR}/../dashboard/adios_common.cmake)
