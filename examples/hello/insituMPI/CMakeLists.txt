#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_insituMPIWriter_f helloInsituMPIWriter.f90 helloInsituArgs.F90)
    target_link_libraries(hello_insituMPIWriter_f MPI::MPI_Fortran)
    add_executable(hello_insituMPIReader_f helloInsituMPIReader.f90 helloInsituArgs.F90)
    target_link_libraries(hello_insituMPIReader_f MPI::MPI_Fortran)
  endif()
  
  add_executable(hello_insituMPIReader helloInsituMPIReader.cpp HelloInsituArgs.cpp)
  target_include_directories(hello_insituMPIReader PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_insituMPIReader MPI::MPI_C)
 
endif()

target_link_libraries(hello_insituMPIReader adios2)

if(ADIOS2_HAVE_Fortran)
  target_link_libraries(hello_insituMPIWriter_f adios2_f)
  target_link_libraries(hello_insituMPIReader_f adios2_f)
endif()

