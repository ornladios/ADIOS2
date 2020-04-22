#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_bpAttributeWriter helloBPAttributeWriter_nompi.cpp)
target_link_libraries(hello_bpAttributeWriter adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  add_executable(hello_bpAttributeWriter_mpi helloBPAttributeWriter.cpp)
  target_link_libraries(hello_bpAttributeWriter_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()

