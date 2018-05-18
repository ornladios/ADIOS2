#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_Fortran)
  add_library(helloInsituArgs OBJECT helloInsituArgs.F90)

  add_executable(hello_insituMPIWriter_f
    helloInsituMPIWriter.f90
    $<TARGET_OBJECTS:helloInsituArgs>
  )
  target_link_libraries(hello_insituMPIWriter_f MPI::MPI_Fortran adios2_f)
  set_property(TARGET hello_insituMPIWriter_f PROPERTY LINKER_LANGUAGE Fortran)

  add_executable(hello_insituMPIReader_f
    helloInsituMPIReader.f90
    $<TARGET_OBJECTS:helloInsituArgs>
  )
  target_link_libraries(hello_insituMPIReader_f MPI::MPI_Fortran adios2_f)
  set_property(TARGET hello_insituMPIReader_f PROPERTY LINKER_LANGUAGE Fortran)
endif()
  
add_executable(hello_insituMPIReader
  helloInsituMPIReader.cpp HelloInsituArgs.cpp
)
target_link_libraries(hello_insituMPIReader MPI::MPI_C adios2)
