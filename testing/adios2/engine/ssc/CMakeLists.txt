#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(SscTest SscTest.cpp)
    target_link_libraries(SscTest adios2 gtest MPI::MPI_C)
    add_test(NAME SscTest COMMAND "mpirun" "-n" "8" $<TARGET_FILE:SscTest>)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(SscTestNoAttributes SscTestNoAttributes.cpp)
    target_link_libraries(SscTestNoAttributes adios2 gtest MPI::MPI_C)
    add_test(NAME SscTestNoAttributes COMMAND "mpirun" "-n" "8" $<TARGET_FILE:SscTestNoAttributes>)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(SscTestNoSelection SscTestNoSelection.cpp)
    target_link_libraries(SscTestNoSelection adios2 gtest MPI::MPI_C)
    add_test(NAME SscTestNoSelection COMMAND "mpirun" "-n" "8" $<TARGET_FILE:SscTestNoSelection>)
endif()
