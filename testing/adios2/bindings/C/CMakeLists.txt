#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteTypes_c TestBPWriteTypes.cpp)
target_link_libraries(TestBPWriteTypes_c adios2 gtest)

add_executable(TestBPWriteReadMultiblock_c TestBPWriteReadMultiblock.cpp)
target_link_libraries(TestBPWriteReadMultiblock_c adios2 gtest)

add_executable(TestNullWriteRead_c TestNullWriteRead.cpp)
target_link_libraries(TestNullWriteRead_c adios2 gtest)

if(ADIOS2_HAVE_MPI)

  add_executable(TestBPWriteAggregateReadLocal_c TestBPWriteAggregateReadLocal.cpp)
  target_link_libraries(TestBPWriteAggregateReadLocal_c adios2 gtest MPI::MPI_C)

  target_link_libraries(TestBPWriteTypes_c MPI::MPI_C)
  target_link_libraries(TestBPWriteReadMultiblock_c MPI::MPI_C)
  target_link_libraries(TestNullWriteRead_c MPI::MPI_C)
  
  set(extra_test_args
    EXEC_WRAPPER
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
  
  gtest_add_tests(TARGET TestBPWriteAggregateReadLocal_c ${extra_test_args})
endif()

gtest_add_tests(TARGET TestBPWriteTypes_c ${extra_test_args})
gtest_add_tests(TARGET TestBPWriteReadMultiblock_c ${extra_test_args})
gtest_add_tests(TARGET TestNullWriteRead_c ${extra_test_args})

