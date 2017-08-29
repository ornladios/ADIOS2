#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# MPI versions of the test are not properly implemented at the moment
if(NOT ADIOS2_HAVE_MPI)
  
  if(ADIOS2_HAVE_ADIOS1)
    add_executable(TestBPWriteRead TestBPWriteRead.cpp)
    target_link_libraries(TestBPWriteRead adios2 gtest adios1::adios)

    add_executable(TestBPWriteReadAttributes TestBPWriteReadAttributes.cpp)
    target_link_libraries(TestBPWriteReadAttributes adios2 gtest adios1::adios)
  
    add_executable(TestBPWriteReadstdio TestBPWriteReadstdio.cpp)
    target_link_libraries(TestBPWriteReadstdio adios2 gtest adios1::adios)

    add_executable(TestBPWriteReadfstream TestBPWriteReadfstream.cpp)
    target_link_libraries(TestBPWriteReadfstream adios2 gtest adios1::adios)
    
    gtest_add_tests(TARGET TestBPWriteRead)
    gtest_add_tests(TARGET TestBPWriteReadAttributes)
    gtest_add_tests(TARGET TestBPWriteReadstdio)
    gtest_add_tests(TARGET TestBPWriteReadfstream)
  endif()
    
  add_executable(TestBPWriteProfilingJSON TestBPWriteProfilingJSON.cpp)
  target_link_libraries(TestBPWriteProfilingJSON
    adios2 gtest gtest_main NLohmannJson
  )
  
  gtest_add_tests(TARGET TestBPWriteProfilingJSON)
endif()

# Testing CMake code
add_executable(TestBPWriteRead TestBPWriteRead.cpp)
target_link_libraries(TestBPWriteRead adios2 gtest adios1::adios)
if(ADIOS2_HAVE_MPI)
  target_include_directories(TestBPWriteRead PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestBPWriteRead ${MPI_C_LIBRARIES})
  set(extra_test_args
    EXEC_WRAPPER
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
endif()

gtest_add_tests(TARGET TestBPWriteRead ${extra_test_args})
