#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_dataSpacesWriter helloDataSpacesWriter.cpp)
target_link_libraries(hello_dataSpacesWriter adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(hello_dataSpacesWriter MPI::MPI_C)
endif()
