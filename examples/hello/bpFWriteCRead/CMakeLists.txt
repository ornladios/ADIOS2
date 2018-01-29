#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(CppWriter CppWriter.cpp)
  target_include_directories(CppWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(CppWriter ${MPI_C_LIBRARIES} adios2)

  add_executable(CppReader CppReader.cpp)
  target_include_directories(CppReader PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(CppReader ${MPI_C_LIBRARIES} adios2)
  
  if(ADIOS2_HAVE_Fortran)
    add_executable(FWriter FWriter.f90)
    target_link_libraries(FWriter MPI::MPI_Fortran)
    target_link_libraries(FWriter adios2_f)
    
    add_executable(FReader FReader.f90)
    target_link_libraries(FReader MPI::MPI_Fortran)
    target_link_libraries(FReader adios2_f)
  endif()
  
else()
  
endif()
