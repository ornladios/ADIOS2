# CI configuration for MemorySanitizer builds

set(ENV{CC}  clang)
set(ENV{CXX} clang++)

set(dashboard_cache "
CMAKE_TOOLCHAIN_FILE:FILEPATH=${CI_SOURCE_DIR}/scripts/ci/images/msan/toolchain-msan.cmake
BUILD_TESTING:BOOL=ON
")

if(NOT CTEST_CMAKE_GENERATOR)
  set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
endif()

# MSan builds are slower, increase timeout
set(CTEST_TEST_TIMEOUT 600)

list(APPEND CTEST_UPDATE_NOTES_FILES "${CMAKE_CURRENT_LIST_FILE}")
include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)
