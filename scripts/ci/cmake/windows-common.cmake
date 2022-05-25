# Client maintainer: chuck.atkins@kitware.com

set(ENV{atl_ROOT} "$ENV{CI_ROOT_DIR}/atl/install")
set(ENV{PATH} "$ENV{CI_ROOT_DIR}/atl/install/bin")

string(APPEND dashboard_cache "
FFS_USE_DILL:BOOL=OFF
")

list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
