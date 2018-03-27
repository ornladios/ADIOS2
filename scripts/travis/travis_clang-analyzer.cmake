# Client maintainer: chuck.atkins@kitware.com
set(CTEST_SITE "TravisCI")
set(CTEST_BUILD_CONFIGURATION Release)
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_FLAGS "-k -j8")
set(CTEST_TEST_ARGS PARALLEL_LEVEL 1)

set(dashboard_model Experimental)
set(dashboard_binary_name "build_clang-analyzer")
set(dashboard_track "Analysis")

set(CTEST_GIT_COMMAND "/usr/bin/git")
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_SOURCE_DIRECTORY "$ENV{SOURCE_DIR}")
set(CTEST_DASHBOARD_ROOT "$ENV{HOME}")

set(ENV{CCC_CC} gcc)
set(ENV{CCC_CXX} g++)
set(ENV{CC}  "$ENV{WRAPPED_CC}")
set(ENV{CXX} "$ENV{WRAPPED_CXX}")

include(${CMAKE_CURRENT_LIST_DIR}/../dashboard/adios_common.cmake)
