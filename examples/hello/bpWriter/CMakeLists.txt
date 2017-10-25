#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpWriter helloBPWriter.cpp)
  target_link_libraries(hello_bpWriter MPI::MPI_C)
  
  add_executable(hello_bpWriter_c helloBPWriter.c)
  target_link_libraries(hello_bpWriter_c MPI::MPI_C)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpWriter_f helloBPWriter.f90)
    target_link_libraries(hello_bpWriter_f MPI::MPI_Fortran)
  endif()
  
  add_executable(hello_bpPutDeferred helloBPPutDeferred.cpp)
  target_link_libraries(hello_bpPutDeferred MPI::MPI_C)
  
else()
  add_executable(hello_bpWriter helloBPWriter_nompi.cpp)
  add_executable(hello_bpWriter_c helloBPWriter_nompi.c)

  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpWriter_f helloBPWriter_nompi.f90)
  endif()
  
  add_executable(hello_bpPutDeferred helloBPPutDeferred_nompi.cpp)
  
endif()

target_link_libraries(hello_bpWriter adios2)
target_link_libraries(hello_bpWriter_c adios2)

if(ADIOS2_HAVE_Fortran)
  target_link_libraries(hello_bpWriter_f adios2_f)
endif()

target_link_libraries(hello_bpPutDeferred adios2)
