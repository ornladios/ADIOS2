#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_inlineReaderWriter helloInlineReaderWriter.cpp)
target_link_libraries(hello_inlineReaderWriter adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(hello_inlineReaderWriter_mpi helloInlineReaderWriter.cpp)
  target_link_libraries(hello_inlineReaderWriter_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
