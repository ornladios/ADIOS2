#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

macro(fortran_add_test_helper testname mpi)
  set(test_targets "")
  set(pfx Bindings.Fortran.)
  set(tgt Test.${pfx}${testname})
  list(APPEND test_targets "${tgt}")
  add_executable(${tgt} Test${testname}.F90)
  set_target_properties(${tgt} PROPERTIES LINKER_LANGUAGE Fortran)
  if(mpi)
    target_link_libraries(${tgt} adios2::fortran_mpi MPI::MPI_Fortran)
    add_test(NAME ${pfx}${testname}
      COMMAND ${MPIEXEC_COMMAND} $<TARGET_FILE:${tgt}>
    )
    set_tests_properties(${pfx}${testname} PROPERTIES
      PROCESSORS "${MPIEXEC_MAX_NUMPROCS}"
    )
  else()
    target_link_libraries(${tgt} adios2::fortran)
    add_test(NAME ${pfx}${testname} COMMAND ${tgt})
  endif()
  set("Test.${pfx}${testname}-TARGETS" "${test_targets}")
endmacro()

if(ADIOS2_HAVE_MPI)
  set(mpi TRUE)
else()
  set(mpi FALSE)
endif()

fortran_add_test_helper(BPWriteTypes ${mpi})

fortran_add_test_helper(Remove ${mpi})

if(mpi)
  fortran_add_test_helper(Adios2BindingsFortranIO TRUE)
  fortran_add_test_helper(BPWriteReadAttributes TRUE)
  fortran_add_test_helper(BPWriteVariableAttributes TRUE) 
  fortran_add_test_helper(BPWriteTypesByName TRUE)
  fortran_add_test_helper(BPWriteTypesLocal TRUE)
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
    PROCESSORS "${MPIEXEC_MAX_NUMPROCS}"
  )
endif()

add_library(SmallTestData_f OBJECT SmallTestData_mod.F90)
add_library(ISmallTestData_f INTERFACE)
target_sources(ISmallTestData_f INTERFACE $<TARGET_OBJECTS:SmallTestData_f>)

foreach(tgt
    ${Test.Bindings.Fortran.BPWriteTypes-TARGETS}
    ${Test.Bindings.Fortran.Remove-TARGETS}
    ${Test.Bindings.Fortran.BPWriteReadAttributes-TARGETS}
    ${Test.Bindings.Fortran.BPWriteVariableAttributes-TARGETS}
    ${Test.Bindings.Fortran.BPWriteTypesByName-TARGETS}
    ${Test.Bindings.Fortran.BPWriteTypesLocal-TARGETS}
    )
  target_link_libraries(${tgt} ISmallTestData_f)
endforeach()
