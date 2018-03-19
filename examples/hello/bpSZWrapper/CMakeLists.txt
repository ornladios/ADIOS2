#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_bpSZWrapper helloBPSZWrapper.cpp)
target_link_libraries(hello_bpSZWrapper adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(hello_bpSZWrapper ${MPI_C_LIBRARIES})
endif()
