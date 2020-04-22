#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

add_executable(TestSstParamFails TestSstParamFails.cpp)
target_link_libraries(TestSstParamFails gtest_interface)
gtest_add_tests(TARGET TestSstParamFails ${extra_test_args})

add_executable(TestSstWriterFails TestSstWriterFails.cpp)
target_link_libraries(TestSstWriterFails gtest_interface)
gtest_add_tests(TARGET TestSstWriterFails ${extra_test_args})

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestSstParamFails adios2::cxx11_mpi MPI::MPI_C)
  target_link_libraries(TestSstWriterFails adios2::cxx11_mpi MPI::MPI_C)
else()
  target_link_libraries(TestSstParamFails adios2::cxx11)
  target_link_libraries(TestSstWriterFails adios2::cxx11)
endif()
