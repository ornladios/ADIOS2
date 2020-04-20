#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(heatTransfer_read_fileonly_adios2 heatRead_adios2.cpp PrintData.h)
target_link_libraries(heatTransfer_read_fileonly_adios2 adios2::cxx11_mpi MPI::MPI_C)

#if(ADIOS2_HAVE_HDF5)
  #add_executable(heatTransfer_read_fileonly_h5_mpi heatRead_h5.cpp PrintData.h)
  #target_link_libraries(heatTransfer_read_fileonly_h5_mpi adios2::cxx11_mpi MPI::MPI_C)
#endif()
