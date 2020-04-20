#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_hdf5Writer helloHDF5Writer_nompi.cpp)
target_link_libraries(hello_hdf5Writer adios2::cxx11)

if(ADIOS2_HAVE_MPI)
  add_executable(hello_hdf5Writer_mpi helloHDF5Writer.cpp)
  target_link_libraries(hello_hdf5Writer_mpi adios2::cxx11_mpi MPI::MPI_C)
endif()
