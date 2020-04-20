#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(insituGlobalArraysWriter insituGlobalArraysWriter.cpp)
target_link_libraries(insituGlobalArraysWriter adios2::cxx11)

add_executable(insituGlobalArraysReaderNxN insituGlobalArraysReaderNxN.cpp)
target_link_libraries(insituGlobalArraysReaderNxN adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(insituGlobalArraysWriter_mpi insituGlobalArraysWriter.cpp)
  target_link_libraries(insituGlobalArraysWriter_mpi adios2::cxx11_mpi MPI::MPI_C)

  add_executable(insituGlobalArraysReaderNxN_mpi insituGlobalArraysReaderNxN.cpp)
  target_link_libraries(insituGlobalArraysReaderNxN_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
