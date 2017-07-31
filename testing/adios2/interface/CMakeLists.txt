#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestADIOSInterfaceWrite TestADIOSInterfaceWrite.cpp)
target_link_libraries(TestADIOSInterfaceWrite adios2 gtest gtest_main)

add_executable(TestADIOSDefineVariable TestADIOSDefineVariable.cpp)
target_link_libraries(TestADIOSDefineVariable adios2 gtest gtest_main)

gtest_add_tests(TARGET TestADIOSInterfaceWrite)
gtest_add_tests(TARGET TestADIOSDefineVariable)