#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# MPI versions of the test are not properly implemented at the moment

if (ADIOS2_HAVE_ADIOS1)
  add_executable(TestBPWriteRead TestBPWriteRead.cpp)
  target_link_libraries(TestBPWriteRead adios2 gtest adios1::adios)

#  add_executable(TestBPWriteReadAttributes TestBPWriteReadAttributes.cpp)
#  target_link_libraries(TestBPWriteReadAttributes adios2 gtest adios1::adios)

  add_executable(TestBPWriteReadstdio TestBPWriteReadstdio.cpp)
  target_link_libraries(TestBPWriteReadstdio adios2 gtest adios1::adios)

  add_executable(TestBPWriteReadfstream TestBPWriteReadfstream.cpp)
  target_link_libraries(TestBPWriteReadfstream adios2 gtest adios1::adios)

  add_executable(TestBPWriteProfilingJSON TestBPWriteProfilingJSON.cpp)
  target_link_libraries(TestBPWriteProfilingJSON
    adios2 gtest NLohmannJson
  )

  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteRead MPI::MPI_C)
#    target_link_libraries(TestBPWriteReadAttributes MPI::MPI_C)
    target_link_libraries(TestBPWriteReadstdio MPI::MPI_C)
    target_link_libraries(TestBPWriteReadfstream MPI::MPI_C)
    target_link_libraries(TestBPWriteProfilingJSON MPI::MPI_C)

    set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
  endif()

  gtest_add_tests(TARGET TestBPWriteRead ${extra_test_args})
#  gtest_add_tests(TARGET TestBPWriteReadAttributes ${extra_test_args})
  gtest_add_tests(TARGET TestBPWriteReadstdio ${extra_test_args})
  gtest_add_tests(TARGET TestBPWriteReadfstream ${extra_test_args})
  gtest_add_tests(TARGET TestBPWriteProfilingJSON ${extra_test_args})
endif()
