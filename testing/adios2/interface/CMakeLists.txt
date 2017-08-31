#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestADIOSInterfaceWrite TestADIOSInterfaceWrite.cpp)
target_link_libraries(TestADIOSInterfaceWrite adios2 gtest gtest_main)

add_executable(TestADIOSDefineVariable TestADIOSDefineVariable.cpp)
target_link_libraries(TestADIOSDefineVariable adios2 gtest gtest_main)

add_executable(TestADIOSDefineAttribute TestADIOSDefineAttribute.cpp)
target_link_libraries(TestADIOSDefineAttribute adios2 gtest gtest_main)

if(ADIOS2_HAVE_MPI)
  target_include_directories(TestADIOSInterfaceWrite PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestADIOSInterfaceWrite ${MPI_C_LIBRARIES})

  target_include_directories(TestADIOSDefineVariable PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestADIOSDefineVariable ${MPI_C_LIBRARIES})

  target_include_directories(TestADIOSDefineAttribute PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestADIOSDefineAttribute ${MPI_C_LIBRARIES})

  set(extra_test_args
    EXEC_WRAPPER
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2#${MPIEXEC_MAX_NUMPROCS}
  )
endif()

gtest_add_tests(TARGET TestADIOSInterfaceWrite)
gtest_add_tests(TARGET TestADIOSDefineVariable)
gtest_add_tests(TARGET TestADIOSDefineAttribute ${extra_test_args})
