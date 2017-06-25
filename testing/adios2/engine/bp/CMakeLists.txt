#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# MPI versions of the test are not properly implemented at the moment
if(NOT ADIOS2_HAVE_MPI)
  find_package(ADIOS1 COMPONENTS sequential REQUIRED)

  add_executable(TestBPWriteRead TestBPWriteRead.cpp)
  target_link_libraries(TestBPWriteRead adios2 gtest adios1::adios)
  
  add_executable(TestBPWriteReadstdio TestBPWriteReadstdio.cpp)
  target_link_libraries(TestBPWriteReadstdio adios2 gtest adios1::adios)

  add_executable(TestBPWriteReadfstream TestBPWriteReadfstream.cpp)
  target_link_libraries(TestBPWriteReadfstream adios2 gtest adios1::adios)
  

  gtest_add_tests(TARGET TestBPWriteRead)
  gtest_add_tests(TARGET TestBPWriteReadstdio)
  gtest_add_tests(TARGET TestBPWriteReadfstream)
endif()
