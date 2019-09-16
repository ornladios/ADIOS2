#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestTableMultiRank TestTableMultiRank.cpp)
    target_link_libraries(TestTableMultiRank adios2 gtest MPI::MPI_C)
    add_test(NAME TestTableMultiRank COMMAND "mpirun" "-n" "4" $<TARGET_FILE:TestTableMultiRank>)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestTableSingleRank TestTableSingleRank.cpp)
    target_link_libraries(TestTableSingleRank adios2 gtest MPI::MPI_C)
    add_test(NAME TestTableSingeRank COMMAND $<TARGET_FILE:TestTableSingleRank>)
endif()
