# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0


# This module is already included in new versions of CMake,
# however, it does not yet support MPI launchers as shown here:
# https://discourse.cmake.org/t/ctest-and-mpi-parallel-googletests/5557
include(${CMAKE_CURRENT_LIST_DIR}/upstream/GoogleTest.cmake)
