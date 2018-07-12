#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(extra_test_args)

add_executable(TestManyVars manyVars.cpp)
target_link_libraries(TestManyVars adios2 gtest_interface)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestManyVars MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestManyVars ${extra_test_args})

if(ADIOS2_HAVE_MPI)
  # Pure C code, not added to test, 
  # just for executing manually for performance studies
  add_executable(PerfManyVars manyVars.c)
  target_link_libraries(PerfManyVars adios2)
  target_link_libraries(PerfManyVars MPI::MPI_C)
endif()
