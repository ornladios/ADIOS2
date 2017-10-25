#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(localArray_write localArray_write.cpp)
target_link_libraries(localArray_write adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(localArray_write MPI::MPI_C)
endif()
