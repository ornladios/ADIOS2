#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(heatTransfer_read_fileonly_adios2 heatRead_adios2.cpp PrintData.h)
  target_link_libraries(heatTransfer_read_fileonly_adios2 adios2 MPI::MPI_C)

#  if(ADIOS2_HAVE_HDF5)
    #add_executable(heatTransfer_read_fileonly_h5 heatRead_h5.cpp PrintData.h)
    #target_link_libraries(heatTransfer_read_fileonly_h5 adios2 MPI::MPI_C)
#  endif()

  if(ADIOS2_HAVE_ADIOS1)
    add_executable(heatTransfer_read_fileonly_adios1 heatRead_adios1.cpp PrintData.h)
    target_link_libraries(heatTransfer_read_fileonly_adios1 adios1::adios MPI::MPI_C)
  endif()
endif()
