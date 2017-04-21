#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_timeBPWriter timeBPWriter.cpp)
  target_include_directories(hello_timeBPWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_timeBPWriter ${MPI_C_LIBRARIES})
else()
  add_executable(hello_timeBPWriter timeBPWriter_nompi.cpp)
endif()
target_link_libraries(hello_timeBPWriter adios2)
