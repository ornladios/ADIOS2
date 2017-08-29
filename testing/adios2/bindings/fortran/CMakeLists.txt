#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteTypes_f SmallTestData_mod.f90)

if(ADIOS2_HAVE_MPI)
  target_sources(TestBPWriteTypes_f PRIVATE TestBPWriteTypes.f90)
  target_include_directories(TestBPWriteTypes_f 
                                        PRIVATE ${MPI_Fortran_INCLUDE_PATH}
                                                ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestBPWriteTypes_f PRIVATE ${MPI_Fortran_LIBRARIES}
                                                   ${MPI_C_LIBRARIES})  
else()
  target_sources(TestBPWriteTypes_f PRIVATE TestBPWriteTypes_nompi.f90)
endif()

target_link_libraries(TestBPWriteTypes_f PRIVATE adios2_f)

add_test(NAME BPWrite_f COMMAND TestBPWriteTypes_f)
