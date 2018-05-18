#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpReader helloBPReader.cpp)
  target_link_libraries(hello_bpReader adios2 MPI::MPI_C)
  
  add_executable(hello_bpReaderHeatMap2D helloBPReaderHeatMap2D.cpp)
  target_link_libraries(hello_bpReaderHeatMap2D adios2 MPI::MPI_C)
  
  add_executable(hello_bpReaderHeatMap3D helloBPReaderHeatMap3D.cpp)
  target_link_libraries(hello_bpReaderHeatMap3D adios2 MPI::MPI_C)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpReaderHeatMap3D_f helloBPReaderHeatMap3D.f90)
    target_link_libraries(hello_bpReaderHeatMap3D_f adios2_f MPI::MPI_Fortran)
    set_property(TARGET hello_bpReaderHeatMap3D_f
      PROPERTY LINKER_LANGUAGE Fortran
    )
  endif()
  
else()
  add_executable(hello_bpReader helloBPReader_nompi.cpp)
  target_link_libraries(hello_bpReader adios2)
endif()
