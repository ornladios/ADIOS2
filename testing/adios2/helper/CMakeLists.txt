# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
#Distributed under the OSI - approved Apache License, Version 2.0. See
#accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(Strings MPI_NONE Helper Helper. "")
gtest_add_tests_helper(DivideBlock MPI_NONE "" Helper. "")
gtest_add_tests_helper(MinMaxs MPI_NONE "" Helper. "")
gtest_add_tests_helper(RangeFilter MPI_NONE "" Helper. "")
gtest_add_tests_helper(ReadNonBPFile MPI_NONE "" Helper. "")
gtest_add_tests_helper(Partitioners MPI_NONE "" Helper. "")

if (ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(RerouteMessage MPI_ONLY "" Helper. "")
endif()
