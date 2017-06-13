#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_bpFlushWriter helloBPFlushWriter.cpp)
  target_include_directories(hello_bpFlushWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpFlushWriter ${MPI_C_LIBRARIES})
else()
  add_executable(hello_bpFlushWriter helloBPFlushWriter_nompi.cpp)
endif()
target_link_libraries(hello_bpFlushWriter adios2)
