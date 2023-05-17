#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(campaign_write campaign_write.cpp)
  target_link_libraries(campaign_write adios2::cxx11_mpi adios2_core MPI::MPI_C)
endif()
