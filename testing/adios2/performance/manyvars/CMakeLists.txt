#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(ManyVars ${test_mpi} "" Performance. "")

if(ADIOS2_HAVE_MPI)
  # Pure C code, not added to test, 
  # just for executing manually for performance studies
  add_executable(PerfManyVars PerfManyVars.c)
  target_link_libraries(PerfManyVars adios2 MPI::MPI_C)
endif()
