# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "TravisCI")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "$ENV{TRAVIS_GENERATOR}")
if(CTEST_CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  set(CTEST_BUILD_FLAGS "-k -j4")
endif()
set(CTEST_TEST_ARGS PARALLEL_LEVEL 4)
set(CTEST_BUILD_NAME "$ENV{TRAVIS_PULL_REQUEST_BRANCH}_$ENV{TRAVIS_BUILD_NUMBER}_$ENV{TRAVIS_BUILD_NAME}")

set(dashboard_model Experimental)
set(dashboard_binary_name "build_$ENV{TRAVIS_JOB_ID}")
set(dashboard_track "Continuous Integration")

set(CTEST_GIT_COMMAND "/usr/bin/git")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{SOURCE_DIR}")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

# TODO: resolve warnings and re-enable Werror
#set(ENV{CFLAGS} -Werror)
#set(ENV{CXXFLAGS} -Werror)
#set(ENV{FFLAGS} -Werror)
find_program(PYTHON_EXECUTABLE python2)

set(dashboard_cache "
#ADIOS2_USE_ADIOS1:STRING=OFF
#ADIOS2_USE_BZip2:STRING=ON
#ADIOS2_USE_DataMan:STRING=ON
#ADIOS2_USE_Fortran:STRING=OFF
#ADIOS2_USE_HDF5:STRING=ON
#ADIOS2_USE_MPI:STRING=OFF
#ADIOS2_USE_Python:STRING=ON
PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
#ADIOS2_USE_ZFP:STRING=OFF
#ADIOS2_USE_ZeroMQ:STRING=ON

MPIEXEC_MAX_NUMPROCS:STRING=4
")

include(${CMAKE_CURRENT_LIST_DIR}/../dashboard/adios_common.cmake)
