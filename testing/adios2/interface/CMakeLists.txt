#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestADIOSInterfaceWrite TestADIOSInterfaceWrite.cpp)
target_link_libraries(TestADIOSInterfaceWrite adios2 gtest gtest_main)
add_test(NAME adios2::interface::write COMMAND TestADIOSInterfaceWrite)
