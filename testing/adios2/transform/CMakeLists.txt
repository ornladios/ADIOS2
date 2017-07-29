#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


if(ADIOS2_HAVE_BZip2)
  add_executable(TestBZip2Wrapper TestBZip2Wrapper.cpp)
  target_link_libraries(TestBZip2Wrapper adios2 gtest gtest_main)

  gtest_add_tests(TARGET TestBZip2Wrapper)
endif()

if(ADIOS2_HAVE_ZFP)
  add_executable(TestZfpWrapper TestZfpWrapper.cpp)
  target_link_libraries(TestZfpWrapper adios2 gtest gtest_main)

  gtest_add_tests(TARGET TestZfpWrapper)
endif()