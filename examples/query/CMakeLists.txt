#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(queryTest test.cpp)
  target_link_libraries(queryTest adios2::cxx11_mpi MPI::MPI_C)
endif()

