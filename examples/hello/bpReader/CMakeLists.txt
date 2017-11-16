#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(hello_bpReader helloBPReader.cpp)
  target_include_directories(hello_bpReader PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpReader ${MPI_C_LIBRARIES})
  
  add_executable(hello_bpReaderHeatMap2D helloBPReaderHeatMap2D.cpp)
  target_include_directories(hello_bpReaderHeatMap2D PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpReaderHeatMap2D ${MPI_C_LIBRARIES})
  target_link_libraries(hello_bpReaderHeatMap2D adios2)
  
  add_executable(hello_bpReaderHeatMap3D helloBPReaderHeatMap3D.cpp)
  target_include_directories(hello_bpReaderHeatMap3D PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_bpReaderHeatMap3D ${MPI_C_LIBRARIES})
  target_link_libraries(hello_bpReaderHeatMap3D adios2)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(hello_bpReaderHeatMap3D_f helloBPReaderHeatMap3D.f90)
    target_link_libraries(hello_bpReaderHeatMap3D_f MPI::MPI_Fortran)
    target_link_libraries(hello_bpReaderHeatMap3D_f adios2_f)
  endif()
  
  
#  add_executable(hello_bpReader_c helloBPReader.c)
#  target_include_directories(hello_bpReader_c PRIVATE ${MPI_C_INCLUDE_PATH})
#  target_link_libraries(hello_bpReader_c ${MPI_C_LIBRARIES})

  
else()
  add_executable(hello_bpReader helloBPReader_nompi.cpp)
#  add_executable(hello_bpReader_c helloBPReader_nompi.c)
#
#  if(ADIOS2_HAVE_Fortran)
#    add_executable(hello_bpReader_f helloBPReader_nompi.f90)
#  endif()
  
endif()

target_link_libraries(hello_bpReader adios2)

#target_link_libraries(hello_bpReader_c adios2)

#if(ADIOS2_HAVE_Fortran)
#  target_link_libraries(hello_bpReader_f PRIVATE adios2_f)
#endif()
