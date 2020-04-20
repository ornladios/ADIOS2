#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_sstWriter helloSstWriter.cpp)
target_link_libraries(hello_sstWriter adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(hello_sstWriter_mpi helloSstWriter.cpp)
  target_link_libraries(hello_sstWriter_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
