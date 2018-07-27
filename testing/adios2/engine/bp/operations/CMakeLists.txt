#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

if(ADIOS2_HAVE_SZ)
  add_executable(TestBPWriteReadSZ TestBPWriteReadSZ.cpp)
  target_link_libraries(TestBPWriteReadSZ adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadSZ MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadSZ ${extra_test_args})
endif()


if(ADIOS2_HAVE_ZFP)
  add_executable(TestBPWriteReadZfp TestBPWriteReadZfp.cpp)
  target_link_libraries(TestBPWriteReadZfp adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadZfp MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadZfp ${extra_test_args})
endif()

