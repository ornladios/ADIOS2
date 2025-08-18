# Client maintainer: eisen@cc.gatech.edu

set(ENV{CC}  nvc)
set(ENV{CXX} nvc++)

list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/unix-common.cmake)
