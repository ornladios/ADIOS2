#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(SmallTestData_f OBJECT SmallTestData_mod.f90)

add_executable(TestBPWriteReadTypes_f $<TARGET_OBJECTS:SmallTestData_f>)
target_link_libraries(TestBPWriteReadTypes_f adios2_f)

add_executable(TestBPWriteReadTypesHighLevelAPI_f
  $<TARGET_OBJECTS:SmallTestData_f>
)
target_link_libraries(TestBPWriteReadTypesHighLevelAPI_f adios2_f)

add_executable(TestRemove_f $<TARGET_OBJECTS:SmallTestData_f>)
target_link_libraries(TestRemove_f adios2_f)

if(ADIOS2_HAVE_MPI)
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes.f90)
  target_link_libraries(TestBPWriteReadTypes_f MPI::MPI_Fortran)
  
  target_sources(TestBPWriteReadTypesHighLevelAPI_f 
                 PRIVATE TestBPWriteTypesHighLevelAPI.f90)
  target_link_libraries(TestBPWriteReadTypesHighLevelAPI_f MPI::MPI_Fortran)

  target_sources(TestRemove_f PRIVATE TestRemove.f90)
  target_link_libraries(TestRemove_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteAttributes_f $<TARGET_OBJECTS:SmallTestData_f>)
  target_link_libraries(TestBPWriteAttributes_f adios2_f)
  target_sources(TestBPWriteAttributes_f PRIVATE TestBPWriteAttributes.f90)
  target_link_libraries(TestBPWriteAttributes_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteTypesByName_f $<TARGET_OBJECTS:SmallTestData_f>)
  target_link_libraries(TestBPWriteTypesByName_f adios2_f)
  target_sources(TestBPWriteTypesByName_f PRIVATE TestBPWriteTypesByName.f90)
  target_link_libraries(TestBPWriteTypesByName_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteLocal_f $<TARGET_OBJECTS:SmallTestData_f>)
  target_link_libraries(TestBPWriteLocal_f adios2_f)
  target_sources(TestBPWriteLocal_f PRIVATE TestBPWriteTypesLocal.f90)
  target_link_libraries(TestBPWriteLocal_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadHeatMap2D_f TestBPWriteReadHeatMap2D.f90)
  target_link_libraries(TestBPWriteReadHeatMap2D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadHeatMap3D_f TestBPWriteReadHeatMap3D.f90)
  target_link_libraries(TestBPWriteReadHeatMap3D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadHeatMap4D_f TestBPWriteReadHeatMap4D.f90)
  target_link_libraries(TestBPWriteReadHeatMap4D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadHeatMap5D_f TestBPWriteReadHeatMap5D.f90)
  target_link_libraries(TestBPWriteReadHeatMap5D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadHeatMap6D_f TestBPWriteReadHeatMap6D.f90)
  target_link_libraries(TestBPWriteReadHeatMap6D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestBPReadGlobalsByName_f TestBPReadGlobalsByName.f90)
  target_link_libraries(TestBPReadGlobalsByName_f adios2_f MPI::MPI_Fortran)
  
  set(test_parameters
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
  
  add_test(NAME BPWriteAttributes_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteAttributes_f>
  )
  
  add_test(NAME BPWriteReadTypesByName_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteTypesByName_f>
  )
  
  add_test(NAME BPWriteLocal_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteLocal_f>
  )
  
  add_test(NAME BPWriteReadHeatMap2D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadHeatMap2D_f>
  )
  
  add_test(NAME BPWriteReadHeatMap3D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadHeatMap3D_f>
  )
  
  add_test(NAME BPWriteReadHeatMap4D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadHeatMap4D_f>
  )
  
  add_test(NAME BPWriteReadHeatMap5D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadHeatMap5D_f>
  )
  
  add_test(NAME BPWriteReadHeatMap6D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadHeatMap6D_f>
  )
  
  add_test(NAME BPWriteReadTypes_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadTypes_f>
  )

  add_test(NAME BPWriteReadTypesHighLevelAPI_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadTypesHighLevelAPI_f>
  )

  add_test(NAME Remove_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestRemove_f>
  )
  
  add_test(NAME BPReadGlobalsByName_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPReadGlobalsByName_f>
  )
  
else()
  
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes_nompi.f90)

  target_sources(TestBPWriteReadTypesHighLevelAPI_f 
                 PRIVATE TestBPWriteTypesHighLevelAPI_nompi.f90)
  
  target_sources(TestRemove_f PRIVATE TestRemove_nompi.f90)

endif()
