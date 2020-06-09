#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(1D MPI_NONE DataMan Engine.DataMan. "")
gtest_add_tests_helper(2DMemSelect MPI_NONE DataMan Engine.DataMan. "")
gtest_add_tests_helper(3DMemSelect MPI_NONE DataMan Engine.DataMan. "")
gtest_add_tests_helper(WriterDoubleBuffer MPI_NONE DataMan Engine.DataMan. "")
gtest_add_tests_helper(WriterSingleBuffer MPI_NONE DataMan Engine.DataMan. "")
gtest_add_tests_helper(ReaderDoubleBuffer MPI_NONE DataMan Engine.DataMan. "")
gtest_add_tests_helper(ReaderSingleBuffer MPI_NONE DataMan Engine.DataMan. "")
# Turning off DataMan.Reliable test as unstable - May 27, 2020
#  gtest_add_tests_helper(Reliable MPI_NONE DataMan Engine.DataMan. "")
