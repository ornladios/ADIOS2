#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(CppWriter CppWriter.cpp)
  target_link_libraries(CppWriter adios2 MPI::MPI_C)

  add_executable(CppReader CppReader.cpp)
  target_link_libraries(CppReader adios2 MPI::MPI_C)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(FWriter FWriter.f90)
    target_link_libraries(FWriter adios2_f MPI::MPI_Fortran)
    set_property(TARGET FWriter PROPERTY LINKER_LANGUAGE Fortran)
    
    add_executable(FReader FReader.f90)
    target_link_libraries(FReader adios2_f MPI::MPI_Fortran)
    set_property(TARGET FReader PROPERTY LINKER_LANGUAGE Fortran)
  endif()
endif()
