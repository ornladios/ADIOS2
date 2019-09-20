#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(Interface ${test_mpi} ADIOS Interface. "")
gtest_add_tests_helper(Write ${test_mpi} ADIOSInterface Interface. "")
gtest_add_tests_helper(DefineVariable ${test_mpi} ADIOS Interface. "")
gtest_add_tests_helper(DefineAttribute ${test_mpi} ADIOS Interface. "")
gtest_add_tests_helper(Selection ${test_mpi} ADIOS Interface. "")
gtest_add_tests_helper(NoMpi FALSE ADIOS Interface. "")
