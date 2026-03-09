# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(SingleRank MPI_NONE Mhs Engine.MHS. "")
gtest_add_tests_helper(MultiRank MPI_ONLY Mhs Engine.MHS. "")
gtest_add_tests_helper(MultiReader MPI_ONLY Mhs Engine.MHS. "")
