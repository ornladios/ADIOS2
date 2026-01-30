# Client maintainer: eisen@cc.gatech.edu

string(APPEND dashboard_cache "
")

# Point cmake to the dependency install directories using _DIR variables
# which point directly to the cmake config files
set(ENV{atl_DIR} "${CMAKE_CURRENT_LIST_DIR}/../../../../atl/install/lib/cmake/atl")
set(ENV{dill_DIR} "${CMAKE_CURRENT_LIST_DIR}/../../../../dill/install/lib/cmake/dill")
set(ENV{ffs_DIR} "${CMAKE_CURRENT_LIST_DIR}/../../../../ffs/install/CMake")

set(CTEST_BUILD_WARNINGS_AS_ERRORS FALSE)

list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
