#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(localArray_write localArray_write.cpp)
target_link_libraries(localArray_write adios2::cxx11)

add_executable(localArray_read localArray_read.cpp)
target_link_libraries(localArray_read adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(localArray_write_mpi localArray_write.cpp)
  target_link_libraries(localArray_write_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
