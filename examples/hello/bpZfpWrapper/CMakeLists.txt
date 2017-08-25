#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpZfpWrapper helloBPZfpWrapper.cpp)
  target_include_directories(hello_bpZfpWrapper PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpZfpWrapper ${MPI_C_LIBRARIES})
  
else()
  add_executable(hello_bpZfpWrapper helloBPZfpWrapper_nompi.cpp)
endif()

target_link_libraries(hello_bpZfpWrapper adios2)
