#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestADIOS1WriteRead TestADIOS1WriteRead.cpp)
target_link_libraries(TestADIOS1WriteRead adios2 gtest adios1::adios)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestADIOS1WriteRead MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestADIOS1WriteRead ${extra_test_args})
