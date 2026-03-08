# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(SstParamFails MPI_ALLOW "" Engine.SST. "")
gtest_add_tests_helper(SstWriterFails MPI_ALLOW "" Engine.SST. "")
