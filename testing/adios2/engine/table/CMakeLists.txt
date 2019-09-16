#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_ZeroMQ)
    add_executable(TestTableBase TableBase.cpp)
    target_link_libraries(TestTableBase adios2 gtest MPI::MPI_C)
    add_test(NAME TableBase COMMAND "mpirun" "--oversubscribe" "-n" "4" $<TARGET_FILE:TestTableBase>)
endif()

