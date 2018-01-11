#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})

add_executable(TestInSituMPIFunctionAssignPeers  TestInSituMPIFunctionAssignPeers.cpp)
target_link_libraries(TestInSituMPIFunctionAssignPeers adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestInSituMPIFunctionAssignPeers ${extra_test_args})

add_executable(TestInSituMPIWriteRead TestInSituMPIWriteRead.cpp)
target_link_libraries(TestInSituMPIWriteRead adios2 gtest MPI::MPI_C)
gtest_add_tests(TARGET TestInSituMPIWriteRead ${extra_test_args} )

#foreach(NUMPROCS RANGE 2 ${MPIEXEC_MAX_NUMPROCS})
#  gtest_add_tests(TARGET TestInSituMPIWriteRead ${extra_test_args} 
#                  EXTRA_ARGS ${NUMPROCS} 
#                  TEST_SUFFIX _${NUMPROCS}_PROCS)
#endforeach()

#  add_executable(TestInSituMPIOneToOne  TestInSituMPIOneToOne.cpp)
#  target_link_libraries(TestInSituMPIOneToOne adios2 gtest MPI::MPI_C)
#  gtest_add_tests(TARGET TestInSituOneToOne ${extra_test_args})


