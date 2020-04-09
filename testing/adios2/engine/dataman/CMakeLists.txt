#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(1D NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(2DMemSelect NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(3DMemSelect NOEXEC DataMan Engine.DataMan. "")
endif()
