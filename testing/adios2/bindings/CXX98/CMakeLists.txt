#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(extra_test_args)
set(extra_link_libs)
if(ADIOS2_HAVE_MPI)
  set(extra_link_libs MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

add_executable(TestBPWriteReadADIOS2_cxx98 TestBPWriteReadADIOS2_cxx98.cpp)
target_link_libraries(TestBPWriteReadADIOS2_cxx98
  adios2_cxx98 gtest_interface ${extra_link_libs})

gtest_add_tests(TARGET TestBPWriteReadADIOS2_cxx98 ${extra_test_args})


