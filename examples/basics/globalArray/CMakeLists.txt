#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(globalArray_write globalArray_write.cpp)
target_link_libraries(globalArray_write adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(globalArray_write_mpi globalArray_write.cpp)
  target_link_libraries(globalArray_write_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
