#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteReadTypes_f SmallTestData_mod.f90)
target_link_libraries(TestBPWriteReadTypes_f adios2_f)

if(ADIOS2_HAVE_MPI)
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes.f90)
  target_link_libraries(TestBPWriteReadTypes_f MPI::MPI_Fortran)
  
  add_executable(TestBPWriteAttributes_f SmallTestData_mod.f90)
  target_link_libraries(TestBPWriteAttributes_f adios2_f)
  target_sources(TestBPWriteAttributes_f PRIVATE TestBPWriteAttributes.f90)
  target_link_libraries(TestBPWriteAttributes_f MPI::MPI_Fortran)
  
  set(test_parameters
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
  
  add_test(NAME BPWriteAttributes_f
    COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteAttributes_f>
  )
  
else()
  target_sources(TestBPWriteReadTypes_f PRIVATE TestBPWriteTypes_nompi.f90)
endif()

add_test(NAME BPWriteReadTypes_f
  COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteReadTypes_f>
)
