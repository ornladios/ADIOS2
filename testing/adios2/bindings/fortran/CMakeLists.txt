#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(SmallTestData_f OBJECT SmallTestData_mod.f90)

add_executable(TestBPWriteReadTypes_f $<TARGET_OBJECTS:SmallTestData_f>)
target_link_libraries(TestBPWriteReadTypes_f adios2_f)

add_executable(TestRemove_f $<TARGET_OBJECTS:SmallTestData_f>)
target_link_libraries(TestRemove_f adios2_f)

if(ADIOS2_HAVE_MPI)
  add_executable(TestAdios2BindingsFortranIO_f TestAdios2BindingsFortranIO.f90)
  target_link_libraries(TestAdios2BindingsFortranIO_f adios2_f MPI::MPI_Fortran)

  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes.f90)
  target_link_libraries(TestBPWriteReadTypes_f MPI::MPI_Fortran)
  
  target_sources(TestRemove_f PRIVATE TestRemove.f90)
  target_link_libraries(TestRemove_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteReadAttributes_f $<TARGET_OBJECTS:SmallTestData_f>)
  target_link_libraries(TestBPWriteReadAttributes_f adios2_f)
  target_sources(TestBPWriteReadAttributes_f PRIVATE TestBPWriteReadAttributes.f90)
  target_link_libraries(TestBPWriteReadAttributes_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteVariableAttributes_f $<TARGET_OBJECTS:SmallTestData_f>)
  target_link_libraries(TestBPWriteVariableAttributes_f adios2_f)
  target_sources(TestBPWriteVariableAttributes_f PRIVATE TestBPWriteVariableAttributes.f90)
  target_link_libraries(TestBPWriteVariableAttributes_f MPI::MPI_Fortran)
  
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
  
  add_executable(TestBPMemorySelection2D_f TestBPWriteMemorySelectionRead2D.f90)
  target_link_libraries(TestBPMemorySelection2D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestBPMemorySelection3D_f TestBPWriteMemorySelectionRead3D.f90)
  target_link_libraries(TestBPMemorySelection3D_f adios2_f MPI::MPI_Fortran)
  
  add_executable(TestNullEngine_f TestNullEngine.f90)
  target_link_libraries(TestNullEngine_f adios2_f MPI::MPI_Fortran)
  
  # F2C 
  add_executable(TestBPReadFBlocks_f2c TestF2C_BPReadFBlocks.cpp)
  target_link_libraries(TestBPReadFBlocks_f2c adios2 gtest MPI::MPI_C)
  
  if(ADIOS2_HAVE_SZ)
    add_executable(TestBPWriteReadSZ2D_f operation/TestBPWriteReadSZ2D.f90)
    target_link_libraries(TestBPWriteReadSZ2D_f adios2_f MPI::MPI_Fortran)
    
    add_executable(TestBPWriteReadSZ3D_f operation/TestBPWriteReadSZ3D.f90)
    target_link_libraries(TestBPWriteReadSZ3D_f adios2_f MPI::MPI_Fortran)
  endif()
  
  if(ADIOS2_HAVE_ZFP)
    add_executable(TestBPWriteReadZfp2D_f operation/TestBPWriteReadZfp2D.f90)
    target_link_libraries(TestBPWriteReadZfp2D_f adios2_f MPI::MPI_Fortran)
  endif()
  
  set(test_parameters
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
  
  add_test(NAME TestAdios2BindingsFortranIO_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestAdios2BindingsFortranIO_f>
  )

  add_test(NAME BPWriteReadAttributes_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadAttributes_f>
  )
  
  add_test(NAME BPWriteVariableAttributes_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteVariableAttributes_f>
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

  add_test(NAME Remove_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestRemove_f>
  )
  
  add_test(NAME BPReadGlobalsByName_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPReadGlobalsByName_f>
  )
  
  add_test(NAME TestBPReadFBlocks_f2c
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPReadFBlocks_f2c>
  )
  
  set_property(TEST TestBPReadFBlocks_f2c
    PROPERTY DEPENDS BPWriteReadHeatMap2D_f BPWriteReadHeatMap3D_f
  )
  
  add_test(NAME TestNullEngine_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestNullEngine_f>
  )
  
  if(ADIOS2_HAVE_SZ)
    add_test(NAME BPWriteReadSZ2D_f
      COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadSZ2D_f>
    )
    
    add_test(NAME BPWriteReadSZ3D_f
      COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadSZ3D_f>
    )
  endif()
  
  if(ADIOS2_HAVE_ZFP)
    add_test(NAME BPWriteReadZfp2D_f
      COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadZfp2D_f>
    )
  endif()
  
  add_test(NAME BPMemorySelection2D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPMemorySelection2D_f>
  )
  
  add_test(NAME BPMemorySelection3D_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPMemorySelection3D_f>
  )
  
else()
  
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes_nompi.f90)
  
  target_sources(TestRemove_f PRIVATE TestRemove_nompi.f90)

endif()
