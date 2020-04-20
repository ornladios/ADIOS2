#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_bpFlushWriter helloBPFlushWriter_nompi.cpp)
target_link_libraries(hello_bpFlushWriter adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpFlushWriter_mpi helloBPFlushWriter.cpp)
  target_link_libraries(hello_bpFlushWriter_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
