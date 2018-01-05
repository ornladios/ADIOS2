#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(BPStreamWrite BPStreamWrite.cpp)
  
  target_link_libraries(BPStreamWrite MPI::MPI_C)
  
else()
  add_executable(BPStreamWrite BPStreamWrite_nompi.cpp)
endif()

target_link_libraries(BPStreamWrite adios2)
