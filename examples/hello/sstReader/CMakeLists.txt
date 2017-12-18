#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_sstReader helloSstReader.cpp)
target_link_libraries(hello_sstReader adios2)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(hello_sstReader MPI::MPI_C)
endif()
