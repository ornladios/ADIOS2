#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(SingleRank FALSE Table Engine.Table. "")

if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(MultiRank "${ADIOS2_HAVE_MPI}" Table Engine.Table. "")
endif()
