#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestBPWriteTypes_c TestBPWriteTypes.cpp)
target_link_libraries(TestBPWriteTypes_c adios2 gtest)

if(ADIOS2_HAVE_MPI)
  target_include_directories(TestBPWriteTypes_c PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestBPWriteTypes_c ${MPI_C_LIBRARIES})
  set(extra_test_args
    EXEC_WRAPPER
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS})
endif()

gtest_add_tests(TARGET TestBPWriteTypes_c ${extra_test_args})
