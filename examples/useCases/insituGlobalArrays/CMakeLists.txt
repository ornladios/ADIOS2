#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(insituGlobalArraysWriter insituGlobalArraysWriter.cpp)
target_link_libraries(insituGlobalArraysWriter adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(insituGlobalArraysWriter MPI::MPI_C)
endif()
