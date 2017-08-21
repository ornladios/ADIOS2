#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpWriter helloBPWriter.cpp)
  target_include_directories(hello_bpWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpWriter ${MPI_C_LIBRARIES})
  
  if(ADIOS2_HAVE_C)
    add_executable(hello_bpWriter_c helloBPWriter.c)
    target_include_directories(hello_bpWriter_c PRIVATE ${MPI_C_INCLUDE_PATH})
    target_link_libraries(hello_bpWriter_c ${MPI_C_LIBRARIES})
  endif()
  
else()
  add_executable(hello_bpWriter helloBPWriter_nompi.cpp)

  if(ADIOS2_HAVE_C)
    add_executable(hello_bpWriter_c helloBPWriter_nompi.c)
  endif()
endif()

target_link_libraries(hello_bpWriter adios2)

if(ADIOS2_HAVE_C)
  target_link_libraries(hello_bpWriter_c adios2_c adios2)
endif()
