#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestHDF5WriteRead
  TestHDF5WriteRead.cpp
  ../EngineWriteReadTest.cpp
)
target_link_libraries(TestHDF5WriteRead adios2 gtest gtest_main)
add_test(NAME adios2::engine::hdf5::write_read COMMAND TestHDF5WriteRead)
