#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_bpReader helloBPReader_nompi.cpp)
target_link_libraries(hello_bpReader adios2::cxx11)

if(ADIOS2_HAVE_Fortran)
  add_executable(hello_bpReaderHeatMap3D_f helloBPReaderHeatMap3D.F90)
  set_property(TARGET hello_bpReaderHeatMap3D_f PROPERTY LINKER_LANGUAGE Fortran)
  target_link_libraries(hello_bpReaderHeatMap3D_f adios2::fortran)
endif()

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpReader_mpi helloBPReader.cpp)
  target_link_libraries(hello_bpReader_mpi adios2::cxx11_mpi MPI::MPI_C)
  
  add_executable(hello_bpReaderHeatMap2D helloBPReaderHeatMap2D.cpp)
  target_link_libraries(hello_bpReaderHeatMap2D adios2::cxx11_mpi MPI::MPI_C)
  
  add_executable(hello_bpReaderHeatMap3D helloBPReaderHeatMap3D.cpp)
  target_link_libraries(hello_bpReaderHeatMap3D adios2::cxx11_mpi MPI::MPI_C)

  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpReaderHeatMap3D_f_mpi helloBPReaderHeatMap3D.F90)
    target_link_libraries(hello_bpReaderHeatMap3D_f_mpi adios2::fortran_mpi MPI::MPI_Fortran)
    set_property(TARGET hello_bpReaderHeatMap3D_f_mpi PROPERTY LINKER_LANGUAGE Fortran)
  endif()
endif()
