#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})

add_executable(TestSstParamFails TestSstParamFails.cpp)
target_link_libraries(TestSstParamFails adios2 gtest_interface MPI::MPI_C)
gtest_add_tests(TARGET TestSstParamFails ${extra_test_args})

add_executable(TestSstWriterFails TestSstWriterFails.cpp)
target_link_libraries(TestSstWriterFails adios2 gtest_interface MPI::MPI_C)
gtest_add_tests(TARGET TestSstWriterFails ${extra_test_args})
