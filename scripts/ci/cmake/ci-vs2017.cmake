# Client maintainer: chuck.atkins@kitware.com

set(ENV{CMAKE_GENERATOR_PLATFORM} "x64")

set(dashboard_cache "
ADIOS2_USE_Python:BOOL=ON
")

set(CTEST_CMAKE_GENERATOR "Visual Studio 15 2017")
list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/ci-common.cmake)
