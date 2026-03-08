# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(ReadOldCompressed MPI_NONE BP4 Engine.BP. .BP4
  WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR} EXTRA_ARGS "BP4"
)

