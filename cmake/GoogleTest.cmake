#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# This module is already included in new versions of CMake,
# however, it does not yet support MPI launchers as shown here:
# https://discourse.cmake.org/t/ctest-and-mpi-parallel-googletests/5557
include(${CMAKE_CURRENT_LIST_DIR}/upstream/GoogleTest.cmake)
