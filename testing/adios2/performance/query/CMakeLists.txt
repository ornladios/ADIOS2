#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(extra_test_args)

add_executable(TestQuery TestBPQuery.cpp)
target_link_libraries(TestQuery adios2 gtest_interface)
if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestQuery MPI::MPI_C)
endif()

gtest_add_tests(TARGET TestQuery ${extra_test_args})

