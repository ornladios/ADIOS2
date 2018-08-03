#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpWriter helloBPWriter.cpp)
  target_link_libraries(hello_bpWriter adios2 MPI::MPI_C)
  
  add_executable(hello_bpWriter_c helloBPWriter.c)
  target_link_libraries(hello_bpWriter_c adios2 MPI::MPI_C)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpWriter_f helloBPWriter.f90)
    target_link_libraries(hello_bpWriter_f adios2_f MPI::MPI_Fortran)
    set_property(TARGET hello_bpWriter_f PROPERTY LINKER_LANGUAGE Fortran)
  endif()
  
  add_executable(hello_bpPutDeferred helloBPPutDeferred.cpp)
  target_link_libraries(hello_bpPutDeferred adios2 MPI::MPI_C)
  
  add_executable(hello_bpSubStreams helloBPSubStreams.cpp)
  target_link_libraries(hello_bpSubStreams adios2 MPI::MPI_C adios2)
  
  if(ADIOS2_HAVE_SZ)
  	add_executable(hello_bpSZ helloBPSZ.cpp)
  	target_link_libraries(hello_bpSZ adios2 MPI::MPI_C adios2)
  endif()
  
else()
  add_executable(hello_bpWriter helloBPWriter_nompi.cpp)
  target_link_libraries(hello_bpWriter adios2)

  add_executable(hello_bpWriter_c helloBPWriter_nompi.c)
  target_link_libraries(hello_bpWriter_c adios2)

  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpWriter_f helloBPWriter_nompi.f90)
    target_link_libraries(hello_bpWriter_f adios2_f)
  endif()
  
  add_executable(hello_bpPutDeferred helloBPPutDeferred_nompi.cpp)
  target_link_libraries(hello_bpPutDeferred adios2)
endif()
