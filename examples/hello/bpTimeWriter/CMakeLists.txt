#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpTimeWriter helloBPTimeWriter.cpp)
  add_executable(hello_bpWriteStep helloBPWriteStep.cpp)
  
  target_include_directories(hello_bpTimeWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpTimeWriter ${MPI_C_LIBRARIES})
else()
  add_executable(hello_bpTimeWriter helloBPTimeWriter_nompi.cpp)
endif()
target_link_libraries(hello_bpTimeWriter adios2)
target_link_libraries(hello_bpWriteStep adios2)