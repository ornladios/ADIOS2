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

if(ADIOS2_HAVE_BZip2)
  add_executable(TestBZip2Wrapper TestBZip2Wrapper.cpp)
  target_link_libraries(TestBZip2Wrapper
    adios2 gtest_interface ${extra_link_libs}
  )

  gtest_add_tests(TARGET TestBZip2Wrapper ${extra_test_args})
endif()

if(ADIOS2_HAVE_ZFP)
  add_executable(TestZfpWrapper TestZfpWrapper.cpp)
  target_link_libraries(TestZfpWrapper
    adios2 gtest_interface ${extra_link_libs}
  )

  gtest_add_tests(TARGET TestZfpWrapper ${extra_test_args})
endif()
