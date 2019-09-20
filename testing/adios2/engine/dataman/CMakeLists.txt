#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(P2P2DMemSelect NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(P2P3DMemSelect NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(P2P1D NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(P2PZfp2D NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(P2PSz2D NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(Subscribe1D NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(MpiBase TRUE DataMan Engine.DataMan. "")
endif()
