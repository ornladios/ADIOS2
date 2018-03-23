option(ADIOS2_USE_SYSTEM_ENET "Use an externally supplied ENET library" OFF)
if(NOT ADIOS2_USE_SYSTEM_ENET)
  set(ENET_LIBRARY_PREFIX adios2_ CACHE INTERNAL "")

  add_subdirectory(enet)
  set(enet_DIR ${CMAKE_CURRENT_BINARY_DIR}/enet CACHE INTERNAL "")
endif()
find_package(enet REQUIRED)
