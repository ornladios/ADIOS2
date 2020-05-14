#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

macro(bp3_bp4_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} "" BackwardCompatibility.250. .BP3
    EXTRA_ARGS ${CMAKE_CURRENT_SOURCE_DIR}/data "BP3"
  )
  gtest_add_tests_helper(${testname} ${mpi} "" BackwardCompatibility.250. .BP4
    EXTRA_ARGS ${CMAKE_CURRENT_SOURCE_DIR}/data "BP4"
  )
endmacro()


bp3_bp4_gtest_add_tests_helper(ReadAttributes MPI_NONE)
bp3_bp4_gtest_add_tests_helper(ReadMultiblocks MPI_NONE)


