#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})

macro(bp3_bp4_gtest_add_tests_helper testname)
  gtest_add_tests_helper(${testname} ${test_mpi} BP Engine.BP. .BP3
    WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
  )
  gtest_add_tests_helper(${testname} ${test_mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
endmacro()

add_subdirectory(operations)

bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2)
bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2fstream)
bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2stdio)
bp3_bp4_gtest_add_tests_helper(WriteReadAsStreamADIOS2)
bp3_bp4_gtest_add_tests_helper(WriteReadAsStreamADIOS2_Threads)
bp3_bp4_gtest_add_tests_helper(WriteReadAttributes)
bp3_bp4_gtest_add_tests_helper(FStreamWriteReadHighLevelAPI)
bp3_bp4_gtest_add_tests_helper(WriteFlushRead)
bp3_bp4_gtest_add_tests_helper(WriteMultiblockRead)
bp3_bp4_gtest_add_tests_helper(WriteReadMultiblock)
bp3_bp4_gtest_add_tests_helper(WriteReadVector)
bp3_bp4_gtest_add_tests_helper(WriteReadAttributesMultirank)
bp3_bp4_gtest_add_tests_helper(LargeMetadata)
bp3_bp4_gtest_add_tests_helper(WriteMemorySelectionRead)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariables)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariablesSel)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariablesSelHighLevel)
bp3_bp4_gtest_add_tests_helper(ChangingShape)
bp3_bp4_gtest_add_tests_helper(WriteReadBlockInfo)
bp3_bp4_gtest_add_tests_helper(WriteReadVariableSpan)

if(ADIOS2_HAVE_MPI)
  bp3_bp4_gtest_add_tests_helper(WriteAggregateRead) 
endif()

# BP3 only for now
gtest_add_tests_helper(WriteNull ${test_mpi} BP Engine.BP. .BP3
  WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
)

# BP4 only for now
gtest_add_tests_helper(WriteAppendReadADIOS2 ${test_mpi} BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
gtest_add_tests_helper(TimeAggregationADIOS2 ${test_mpi} BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
