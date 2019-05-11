#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(WdmTest WdmTest.cpp)
    target_link_libraries(WdmTest adios2 gtest MPI::MPI_C)
    add_test(NAME WdmTest COMMAND "mpirun" "-n" "8" $<TARGET_FILE:WdmTest>)
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(WdmTestNoAttributes WdmTestNoAttributes.cpp)
    target_link_libraries(WdmTestNoAttributes adios2 gtest MPI::MPI_C)
    add_test(NAME WdmTestNoAttributes COMMAND "mpirun" "-n" "8" $<TARGET_FILE:WdmTestNoAttributes>)
endif()


