#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteTypes_f SmallTestData_mod.f90)
target_link_libraries(TestBPWriteTypes_f adios2_f)

if(ADIOS2_HAVE_MPI)
  target_sources(TestBPWriteTypes_f PRIVATE TestBPWriteTypes.f90)
  target_link_libraries(TestBPWriteTypes_f MPI::MPI_Fortran)
  set(test_parameters
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
else()
  target_sources(TestBPWriteTypes_f PRIVATE TestBPWriteTypes_nompi.f90)
endif()

add_test(NAME BPWrite_f
  COMMAND ${test_parameters} $<TARGET_FILE:TestBPWriteTypes_f>
)
