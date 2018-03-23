option(ADIOS2_USE_SYSTEM_EVPath "Use an externally supplied EVPath library" OFF)
if(NOT ADIOS2_USE_SYSTEM_EVPath)
  set(EVPATH_LIBRARY_PREFIX adios2_ CACHE INTERNAL "")

  add_subdirectory(EVPath)
  set(EVPath_DIR ${CMAKE_CURRENT_BINARY_DIR}/EVPath CACHE INTERNAL "")
endif()
find_package(EVPath REQUIRED)
