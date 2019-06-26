#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestNullCoreWrite TestNullCoreWrite.cpp)
target_link_libraries(TestNullCoreWrite adios2 gtest)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestNullCoreWrite MPI::MPI_C)
  
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestNullCoreWrite ${extra_test_args} WORKING_DIRECTORY)
