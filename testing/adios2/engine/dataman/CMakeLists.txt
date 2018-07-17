#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestDataMan TestDataMan.cpp)
target_link_libraries(TestDataMan adios2 gtest)

if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataMan MPI::MPI_C)
endif()

gtest_add_tests(TARGET TestDataMan)

