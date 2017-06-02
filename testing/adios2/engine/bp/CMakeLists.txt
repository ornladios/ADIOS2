#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# MPI versions of the test are not properly implemented at the moment
if(NOT ADIOS_USE_MPI)
  find_package(ADIOS1 COMPONENTS sequential REQUIRED)

  add_executable(TestBPWriteRead TestBPWriteRead.cpp)
  target_link_libraries(TestBPWriteRead adios2 gtest adios1::adios)

  gtest_add_tests(TARGET TestBPWriteRead)
endif()
