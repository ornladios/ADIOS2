#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(1D MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(2DMemSelect MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(3DMemSelect MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(WriterDoubleBuffer MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(WriterSingleBuffer MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(ReaderDoubleBuffer MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(ReaderSingleBuffer MPI_NOEXEC DataMan Engine.DataMan. "")
  gtest_add_tests_helper(Reliable MPI_NOEXEC DataMan Engine.DataMan. "")
endif()
