#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(values_write values_write.cpp)
target_link_libraries(values_write adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(values_write MPI::MPI_C)
endif()
