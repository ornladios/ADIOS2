#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT ADIOS2_HAVE_MPI)
    add_executable(TestBPWriteTypes_c TestBPWriteTypes_nompi.c)
    target_link_libraries(TestBPWriteTypes_c adios2_c adios2)
endif()

if(ADIOS2_HAVE_MPI)
    add_executable(TestBPWriteTypes_c TestBPWriteTypes.c)
    target_include_directories(TestBPWriteTypes_c PRIVATE ${MPI_C_INCLUDE_PATH})
    target_link_libraries(TestBPWriteTypes_c adios2_c adios2 ${MPI_C_LIBRARIES})
endif()

add_test(NAME BPWrite_c COMMAND TestBPWriteTypes_c)