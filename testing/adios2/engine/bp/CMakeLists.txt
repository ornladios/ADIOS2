#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWrite TestBPWrite.cpp)
target_link_libraries(TestBPWrite adios2 gtest gtest_main)
add_test(NAME adios2::engine::bp::write COMMAND TestBPWrite)
