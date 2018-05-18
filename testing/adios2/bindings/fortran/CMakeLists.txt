#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(SmallTestData_f OBJECT SmallTestData_mod.f90)

if(ADIOS2_HAVE_MPI)
  set(fortran_tests
    BPWriteReadTypes BPWriteReadTypesHighLevelAPI Remove
    BPWriteAttributes BPWriteTypesByName BPWriteStep BPWriteLocal
  )

  add_executable(TestBPWriteReadTypes_f TestBPWriteTypes.f90)
  target_link_libraries(TestBPWriteReadTypes_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadTypesHighLevelAPI_f 
    TestBPWriteTypesHighLevelAPI.f90
  )
  target_link_libraries(TestBPWriteReadTypesHighLevelAPI_f MPI::MPI_Fortran)

  add_executable(TestRemove_f TestRemove.f90)
  target_link_libraries(TestRemove_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteAttributes_f TestBPWriteAttributes.f90)
  target_link_libraries(TestBPWriteAttributes_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteTypesByName_f TestBPWriteTypesByName.f90)
  target_link_libraries(TestBPWriteTypesByName_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteStep_f TestBPWriteStep.f90)
  target_link_libraries(TestBPWriteStep_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteLocal_f TestBPWriteTypesLocal.f90)
  target_link_libraries(TestBPWriteLocal_f MPI::MPI_Fortran)
  
  set(test_parameters
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
  
else()
  set(fortran_tests BPWriteReadTypes BPWriteReadTypesHighLevelAPI Remove)

  add_executable(TestBPWriteReadTypes_f TestBPWriteTypes_nompi.f90)
  add_executable(TestBPWriteReadTypesHighLevelAPI_f
    TestBPWriteTypesHighLevelAPI_nompi.f90
  )
  add_executable(TestRemove_f TestRemove_nompi.f90)
endif()

foreach(test IN LISTS fortran_tests)
  target_sources(Test${test}_f PRIVATE $<TARGET_OBJECTS:SmallTestData_f>)
  target_link_libraries(Test${test}_f adios2_f)
  set_property(TARGET Test${test}_f PROPERTY LINKER_LANGUAGE Fortran)

  add_test(
    NAME ${test}_f
    COMMAND ${test_parameters} $<TARGET_FILE:Test${test}_f>
  )
endforeach()
