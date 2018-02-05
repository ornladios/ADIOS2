#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpSZWrapper helloBPSZWrapper.cpp)
  target_link_libraries(hello_bpSZWrapper MPI::MPI_C)
endif()

target_link_libraries(hello_bpSZWrapper adios2)
