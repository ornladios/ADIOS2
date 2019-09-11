#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(test_targets)

macro(add_fortran_test testname)
  add_executable(Test${testname}_f Test${testname}.F90)
  target_link_libraries(Test${testname}_f adios2_f)
  if(ADIOS2_HAVE_MPI)
    target_compile_definitions(Test${testname}_f PRIVATE USE_MPI)
    target_link_libraries(Test${testname}_f MPI::MPI_Fortran)
  endif()
  add_test(NAME Bindings.Fortran.${testname}
    COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:Test${testname}_f>
  )
  set_tests_properties(Bindings.Fortran.${testname} PROPERTIES
    PROCESSORS ${NUM_TEST_PROCS}
  )
endmacro()

add_library(SmallTestData_f OBJECT SmallTestData_mod.F90)
add_library(ISmallTestData_f INTERFACE)
target_sources(ISmallTestData_f INTERFACE $<TARGET_OBJECTS:SmallTestData_f>)

add_fortran_test(BPWriteTypes)
target_link_libraries(TestBPWriteTypes_f ISmallTestData_f)

add_fortran_test(Remove)
target_link_libraries(TestRemove_f ISmallTestData_f)

if(ADIOS2_HAVE_MPI)
  add_fortran_test(Adios2BindingsFortranIO)
  
  add_fortran_test(BPWriteReadAttributes)
  target_link_libraries(TestBPWriteReadAttributes_f ISmallTestData_f)

  add_fortran_test(BPWriteVariableAttributes) 
  target_link_libraries(TestBPWriteVariableAttributes_f ISmallTestData_f)

  add_fortran_test(BPWriteTypesByName)
  target_link_libraries(TestBPWriteTypesByName_f ISmallTestData_f)
 
  add_fortran_test(BPWriteTypesLocal)
  target_link_libraries(TestBPWriteTypesLocal_f ISmallTestData_f)
  
  add_fortran_test(BPWriteReadHeatMap2D)
  add_fortran_test(BPWriteReadHeatMap3D)
  add_fortran_test(BPWriteReadHeatMap4D)
  add_fortran_test(BPWriteReadHeatMap5D)
  add_fortran_test(BPWriteReadHeatMap6D)
  add_fortran_test(BPReadGlobalsByName)
  add_fortran_test(BPWriteMemorySelectionRead2D)
  add_fortran_test(BPWriteMemorySelectionRead3D)
  add_fortran_test(NullEngine)
  
  if(ADIOS2_HAVE_SZ)
    add_fortran_test(BPWriteReadSZ2D)
    add_fortran_test(BPWriteReadSZ3D)
  endif()
  
  if(ADIOS2_HAVE_ZFP)
    add_fortran_test(BPWriteReadZfp2D)
  endif()
  
  # F2C 
  add_executable(TestBPReadFBlocks_f2c TestF2C_BPReadFBlocks.cpp)
  target_link_libraries(TestBPReadFBlocks_f2c adios2 gtest MPI::MPI_C)
  add_test(NAME Bindings.Fortran.F2C.BPReadFBlocks
    COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:TestBPReadFBlocks_f2c>
  )
  set_tests_properties(Bindings.Fortran.F2C.BPReadFBlocks PROPERTIES
    DEPENDS "Bindings.Fortran.BPWriteReadHeatMap2D;Bindings.Fortran.BPWriteReadHeatMap3D"
    PROCESSORS ${NUM_TEST_PROCS}
  )
endif()
