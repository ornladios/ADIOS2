#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_adios1Writer helloADIOS1Writer.cpp)
  target_link_libraries(hello_adios1Writer MPI::MPI_C)
else()
  add_executable(hello_adios1Writer helloADIOS1Writer_nompi.cpp)
endif()
target_link_libraries(hello_adios1Writer adios2)
