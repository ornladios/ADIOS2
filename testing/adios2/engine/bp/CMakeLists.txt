#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteRead
  TestBPWriteRead.cpp
  ../EngineWriteReadTest.cpp
)
target_link_libraries(TestBPWriteRead adios2 gtest gtest_main)
add_test(NAME adios2::engine::bp::write_read COMMAND TestBPWriteRead)
