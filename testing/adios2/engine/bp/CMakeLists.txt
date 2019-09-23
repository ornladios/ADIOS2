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

bp3_bp4_gtest_add_tests_helper(BPWriteReadADIOS2)
bp3_bp4_gtest_add_tests_helper(BPWriteReadADIOS2fstream)
bp3_bp4_gtest_add_tests_helper(BPWriteReadADIOS2stdio)
bp3_bp4_gtest_add_tests_helper(BPWriteReadAsStreamADIOS2)
bp3_bp4_gtest_add_tests_helper(BPWriteReadAsStreamADIOS2_Threads)
bp3_bp4_gtest_add_tests_helper(BPWriteReadAttributesADIOS2)
bp3_bp4_gtest_add_tests_helper(StreamWriteReadHighLevelAPI)
bp3_bp4_gtest_add_tests_helper(BPWriteFlushRead)
bp3_bp4_gtest_add_tests_helper(BPWriteMultiblockRead)
bp3_bp4_gtest_add_tests_helper(BPWriteReadMultiblock)
bp3_bp4_gtest_add_tests_helper(BPWriteReadVector)
bp3_bp4_gtest_add_tests_helper(BPWriteReadAttributesMultirank)
bp3_bp4_gtest_add_tests_helper(BPLargeMetadata)
bp3_bp4_gtest_add_tests_helper(BPWriteMemorySelectionRead)
bp3_bp4_gtest_add_tests_helper(BPWriteReadLocalVariables)
bp3_bp4_gtest_add_tests_helper(BPWriteReadLocalVariablesSel)
bp3_bp4_gtest_add_tests_helper(BPWriteReadLocalVariablesSelHighLevel)
bp3_bp4_gtest_add_tests_helper(BPChangingShape)
bp3_bp4_gtest_add_tests_helper(BPWriteReadBlockInfo)
bp3_bp4_gtest_add_tests_helper(BPWriteReadVariableSpan)

if(ADIOS2_HAVE_MPI)
  bp3_bp4_gtest_add_tests_helper(WriteAggregateRead) 
endif()

# BP3 only for now
#gtest_add_tests_helper(BPWriteReadVariableSpan ${test_mpi} Engine.BP. .BP3
#  WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
#)
gtest_add_tests_helper(BPWriteNull ${test_mpi} Engine.BP. .BP3
  WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
)

# BP4 only for now
gtest_add_tests_helper(WriteAppendReadADIOS2 ${test_mpi} BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
gtest_add_tests_helper(TimeAggregationADIOS2 ${test_mpi} BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
