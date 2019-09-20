#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_Table)
  gtest_add_tests_helper(MultiRank TRUE Table Engine.Table. "")

  add_executable(Test.Engine.Table.SingleRank TestTableSingleRank.cpp)
  target_link_libraries(Test.Engine.Table.SingleRank
    adios2 gtest MPI::MPI_C
  )
  gtest_add_tests(TARGET Test.Engine.Table.SingleRank
    TEST_PREFIX Engine.Table.
    EXEC_WRAPPER ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
  )
endif()
