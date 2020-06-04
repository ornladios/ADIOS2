# This file exists to support multiple installations of ADIOS. Since they can't
# all exist in the same CMake directory, they have to be placed into different
# directories. This file tries to figure out which one the user wants. If it's
# wrong, they can always pass -Dadios2_DIR to CMake with the correct one.

# Try to do something sensible based on the presence or absence of MPI
# variables.
if(MPI_C_INCLUDE_DIRS MATCHES "openmpi" AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/openmpi/adios2-config.cmake")
  include("${CMAKE_CURRENT_LIST_DIR}/openmpi/adios2-config.cmake")
else()
  include("${CMAKE_CURRENT_LIST_DIR}/serial/adios2-config.cmake")
endif()
