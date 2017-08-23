#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT ADIOS2_HAVE_MPI)
    add_executable(TestBPWriteTypes_f TestBPWriteTypes_nompi.f90 SmallTestData_mod.f90)
    target_link_libraries(TestBPWriteTypes_f adios2_f adios2_c adios2)
endif()

if(ADIOS2_HAVE_MPI)
    add_executable(TestBPWriteTypes_f TestBPWriteTypes.f90 SmallTestData_mod.f90)
    target_include_directories(TestBPWriteTypes_f PRIVATE ${MPI_Fortran_INCLUDE_PATH} ${MPI_C_INCLUDE_PATH})
    target_link_libraries(TestBPWriteTypes_f adios2_f adios2_c adios2 ${MPI_Fortran_LIBRARIES} ${MPI_C_LIBRARIES})
endif()

add_test(NAME BPWrite_f COMMAND TestBPWriteTypes_f)