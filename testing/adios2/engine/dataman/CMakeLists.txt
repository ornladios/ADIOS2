#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestDataManEngine TestDataManEngine.cpp)
add_executable(TestDataManFormat TestDataManFormat.cpp)

target_link_libraries(TestDataManEngine adios2 gtest)
target_link_libraries(TestDataManFormat adios2 gtest)

if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestDataManEngine MPI::MPI_C)
    target_link_libraries(TestDataManFormat MPI::MPI_C)
endif()

gtest_add_tests(TARGET TestDataManEngine)
gtest_add_tests(TARGET TestDataManFormat)

