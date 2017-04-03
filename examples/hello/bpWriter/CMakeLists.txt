#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  add_executable(hello_bpWriter helloBPWriter.cpp)
  target_include_directories(hello_bpWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpWriter adios2 ${MPI_C_LIBRARIES})
endif()

add_executable(hello_bpWriter_nompi helloBPWriter_nompi.cpp)
target_link_libraries(hello_bpWriter_nompi adios2)
