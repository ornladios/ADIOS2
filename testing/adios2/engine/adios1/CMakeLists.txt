#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestADIOS1WriteRead
  TestADIOS1WriteRead.cpp
  ../EngineWriteReadTest.cpp
)
target_link_libraries(TestADIOS1WriteRead adios2 gtest)
add_test(NAME adios2::engine::adios1::write_read COMMAND TestADIOS1WriteRead)
