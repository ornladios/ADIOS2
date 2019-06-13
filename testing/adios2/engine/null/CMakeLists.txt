#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


add_executable(TestNullWriteRead TestNullWriteRead.cpp)
target_link_libraries(TestNullWriteRead adios2 gtest)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestNullWriteRead MPI::MPI_C)
  
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif(ADIOS2_HAVE_MPI)

gtest_add_tests(TARGET TestNullWriteRead ${extra_test_args})

