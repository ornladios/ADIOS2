# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "CircleCI")
set(CTEST_BUILD_CONFIGURATION Debug)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j4")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 4)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_$ENV{CIRCLE_JOB}")
set(dashboard_track "Analysis")

set(CTEST_GIT_COMMAND "/usr/bin/git")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{CIRCLE_WORKING_DIRECTORY}/source")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

set(ENV{CC}  clang)
set(ENV{CXX} clang++)
set(ENV{CFLAGS} -fsanitize=address -pthread)
set(ENV{CXXFLAGS} -fsanitize=address -pthread)
set(ENV{FFLAGS} -fsanitize=address -pthread)

set(dashboard_cache "
ADIOS2_USE_BZip2:STRING=ON
ADIOS2_USE_DataMan:STRING=ON
ADIOS2_USE_HDF5:STRING=ON
ADIOS2_USE_MPI:STRING=OFF
ADIOS2_USE_Python:STRING=OFF
ADIOS2_USE_ZFP:STRING=OFF
ADIOS2_USE_ZeroMQ:STRING=ON
")

include(${CMAKE_CURRENT_LIST_DIR}/../../dashboard/adios_common.cmake)
