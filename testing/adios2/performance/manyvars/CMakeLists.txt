#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestManyVars manyVars.cpp)
target_link_libraries(TestManyVars adios2 gtest gtest_main)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestManyVars MPI::MPI_C)
endif()

gtest_add_tests(TARGET TestManyVars)

# Pure C code, not added to test, 
# just for executing manually for performance studies
add_executable(PerfManyVars manyVars.c)
target_link_libraries(PerfManyVars adios2)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(PerfManyVars MPI::MPI_C)
endif()