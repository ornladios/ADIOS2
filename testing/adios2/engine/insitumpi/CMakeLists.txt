#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})

add_executable(TestInSituMPIFunctionAssignPeers  TestInSituMPIFunctionAssignPeers.cpp)
target_link_libraries(TestInSituMPIFunctionAssignPeers adios2 gtest_interface MPI::MPI_C)
gtest_add_tests(TARGET TestInSituMPIFunctionAssignPeers ${extra_test_args})



