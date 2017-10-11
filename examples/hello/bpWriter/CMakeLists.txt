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
    target_link_libraries(hello_bpWriter_f MPI::MPI_Fortran adios2_f)
  endif()

else()
  add_executable(hello_bpWriter helloBPWriter_nompi.cpp)
  add_executable(hello_bpWriter_c helloBPWriter_nompi.c)

  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpWriter_f helloBPWriter_nompi.f90)
    target_link_libraries(hello_bpWriter_f adios2_f)
  endif()
endif()

target_link_libraries(hello_bpWriter adios2)
target_link_libraries(hello_bpWriter_c adios2)

if(ADIOS2_HAVE_Python)
  add_executable(examplePythonPlugin examplePythonPlugin.cpp)
  target_link_libraries(examplePythonPlugin PUBLIC adios2)
endif()
