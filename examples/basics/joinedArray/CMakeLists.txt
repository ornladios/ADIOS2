#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(joinedArray_write joinedArray_write.cpp)
target_link_libraries(joinedArray_write adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(joinedArray_write_mpi joinedArray_write.cpp)
  target_link_libraries(joinedArray_write_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
