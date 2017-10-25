#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpTimeWriter helloBPTimeWriter.cpp)
  add_executable(hello_bpWriteStep helloBPWriteStep.cpp)
  
  target_link_libraries(hello_bpTimeWriter MPI::MPI_C)
  target_link_libraries(hello_bpWriteStep MPI::MPI_C)
  
else()
  add_executable(hello_bpTimeWriter helloBPTimeWriter_nompi.cpp)
  add_executable(hello_bpWriteStep helloBPWriteStep_nompi.cpp)
endif()

target_link_libraries(hello_bpTimeWriter adios2)
target_link_libraries(hello_bpWriteStep adios2)
