# Client maintainer: eisen@cc.gatech.edu

set(dashboard_cache "
")

set(CTEST_CMAKE_GENERATOR "Visual Studio 17")
set(CTEST_CMAKE_GENERATOR_PLATFORM x64)
set(CTEST_CMAKE_GENERATOR_TOOLSET ClangCL)

list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/windows-common.cmake)
