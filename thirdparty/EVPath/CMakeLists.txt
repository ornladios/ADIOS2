message_start_thirdparty()

set(BUILD_TESTING OFF)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_CMAKEDIR}/thirdparty")

set(EVPATH_LIBRARY_PREFIX adios2_ CACHE INTERNAL "")
set(EVPATH_QUIET ON CACHE INTERNAL "")

add_subdirectory(EVPath)
set(EVPath_DIR ${CMAKE_CURRENT_BINARY_DIR}/EVPath CACHE INTERNAL "")

message_end_thirdparty()
