#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

macro(fortran_add_test_helper testname mpi)
  set(pfx Bindings.Fortran.)
  set(tgt Test.${pfx}${testname})
  add_executable(${tgt} Test${testname}.F90)
  target_link_libraries(${tgt} adios2_f)
  if(mpi)
    target_compile_definitions(${tgt} PRIVATE USE_MPI)
    target_link_libraries(${tgt} MPI::MPI_Fortran)
    add_test(NAME ${pfx}${testname}
      COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:${tgt}>
    )
    set_tests_properties(${pfx}${testname} PROPERTIES
      PROCESSORS ${NUM_TEST_PROCS}
    )
  else()
    add_test(NAME ${pfx}${testname} COMMAND ${tgt})
  endif()
endmacro()

if(ADIOS2_HAVE_MPI)
  set(mpi TRUE)
else()
  set(mpi FALSE)
endif()

add_library(SmallTestData_f OBJECT SmallTestData_mod.F90)
add_library(ISmallTestData_f INTERFACE)
target_sources(ISmallTestData_f INTERFACE $<TARGET_OBJECTS:SmallTestData_f>)

fortran_add_test_helper(BPWriteTypes ${mpi})
target_link_libraries(Test.Bindings.Fortran.BPWriteTypes ISmallTestData_f)

fortran_add_test_helper(Remove ${mpi})
target_link_libraries(Test.Bindings.Fortran.Remove ISmallTestData_f)

if(mpi)
  fortran_add_test_helper(Adios2BindingsFortranIO TRUE)
  
  fortran_add_test_helper(BPWriteReadAttributes TRUE)
  target_link_libraries(Test.Bindings.Fortran.BPWriteReadAttributes
    ISmallTestData_f
  )

  fortran_add_test_helper(BPWriteVariableAttributes TRUE) 
  target_link_libraries(Test.Bindings.Fortran.BPWriteVariableAttributes
    ISmallTestData_f
  )

  fortran_add_test_helper(BPWriteTypesByName TRUE)
  target_link_libraries(Test.Bindings.Fortran.BPWriteTypesByName
    ISmallTestData_f
  )
 
  fortran_add_test_helper(BPWriteTypesLocal TRUE)
  target_link_libraries(Test.Bindings.Fortran.BPWriteTypesLocal
    ISmallTestData_f
  )
  
  fortran_add_test_helper(BPWriteReadHeatMap2D TRUE)
  fortran_add_test_helper(BPWriteReadHeatMap3D TRUE)
  fortran_add_test_helper(BPWriteReadHeatMap4D TRUE)
  fortran_add_test_helper(BPWriteReadHeatMap5D TRUE)
  fortran_add_test_helper(BPWriteReadHeatMap6D TRUE)
  fortran_add_test_helper(BPReadGlobalsByName TRUE)
  fortran_add_test_helper(BPWriteMemorySelectionRead2D TRUE)
  fortran_add_test_helper(BPWriteMemorySelectionRead3D TRUE)
  fortran_add_test_helper(NullEngine TRUE)

  add_subdirectory(operation) 
  
  # F2C 
  add_executable(Test.Bindings.Fortran.F2C.BPReadFBlocks
    TestF2C_BPReadFBlocks.cpp
  )
  target_link_libraries(Test.Bindings.Fortran.F2C.BPReadFBlocks
    adios2 gtest MPI::MPI_C
  )
  add_test(NAME Bindings.Fortran.F2C.BPReadFBlocks
    COMMAND
      ${MPIEXEC_COMMAND} $<TARGET_FILE:Test.Bindings.Fortran.F2C.BPReadFBlocks>
  )
  set_tests_properties(Bindings.Fortran.F2C.BPReadFBlocks PROPERTIES
    DEPENDS "Bindings.Fortran.BPWriteReadHeatMap2D;Bindings.Fortran.BPWriteReadHeatMap3D"
    PROCESSORS ${NUM_TEST_PROCS}
  )
endif()
