#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(Interface MPI_ALLOW ADIOS Interface. "")
gtest_add_tests_helper(Write MPI_ALLOW ADIOSInterface Interface. "")
gtest_add_tests_helper(DefineVariable MPI_ALLOW ADIOS Interface. "")
gtest_add_tests_helper(DefineAttribute MPI_ALLOW ADIOS Interface. "")
gtest_add_tests_helper(Selection MPI_NONE ADIOS Interface. "")
gtest_add_tests_helper(NoMpi MPI_NONE ADIOS Interface. "")
