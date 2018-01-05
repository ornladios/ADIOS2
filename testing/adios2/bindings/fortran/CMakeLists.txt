#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteReadTypes_f SmallTestData_mod.f90)
target_link_libraries(TestBPWriteReadTypes_f adios2_f)

add_executable(TestBPWriteReadTypesHighLevelAPI_f SmallTestData_mod.f90)
target_link_libraries(TestBPWriteReadTypesHighLevelAPI_f adios2_f)

if(ADIOS2_HAVE_MPI)
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes.f90)
  target_link_libraries(TestBPWriteReadTypes_f MPI::MPI_Fortran)
  
  target_sources(TestBPWriteReadTypesHighLevelAPI_f 
                 PRIVATE TestBPWriteTypesHighLevelAPI.f90)
  target_link_libraries(TestBPWriteReadTypesHighLevelAPI_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteAttributes_f SmallTestData_mod.f90)
  target_link_libraries(TestBPWriteAttributes_f adios2_f)
  target_sources(TestBPWriteAttributes_f PRIVATE TestBPWriteAttributes.f90)
  target_link_libraries(TestBPWriteAttributes_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteTypesByName_f SmallTestData_mod.f90)
  target_link_libraries(TestBPWriteTypesByName_f adios2_f)
  target_sources(TestBPWriteTypesByName_f PRIVATE TestBPWriteTypesByName.f90)
  target_link_libraries(TestBPWriteTypesByName_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteStep_f SmallTestData_mod.f90)
  target_link_libraries(TestBPWriteStep_f adios2_f)
  target_sources(TestBPWriteStep_f PRIVATE TestBPWriteStep.f90)
  target_link_libraries(TestBPWriteStep_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteLocal_f SmallTestData_mod.f90)
  target_link_libraries(TestBPWriteLocal_f adios2_f)
  target_sources(TestBPWriteLocal_f PRIVATE TestBPWriteTypesLocal.f90)
  target_link_libraries(TestBPWriteLocal_f MPI::MPI_Fortran)
  
  set(test_parameters
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
  
  add_test(NAME BPWriteAttributes_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteAttributes_f>
  )
  
  add_test(NAME BPWriteReadTypesByName_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteTypesByName_f>
  )
  
  add_test(NAME BPWriteStep_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteStep_f>
  )
  
    add_test(NAME BPWriteLocal_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteLocal_f>
  )
  
else()
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes_nompi.f90)
  target_sources(TestBPWriteReadTypesHighLevelAPI_f 
                 PRIVATE TestBPWriteTypesHighLevelAPI_nompi.f90)
endif()

add_test(NAME BPWriteReadTypes_f
  COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadTypes_f>
)

add_test(NAME BPWriteReadTypesHighLevelAPI_f
  COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadTypesHighLevelAPI_f>
)
