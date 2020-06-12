#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})

macro(bp3_bp4_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP3
    WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
endmacro()

add_subdirectory(operations)

bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2 MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2fstream MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2stdio MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadAsStreamADIOS2 MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadAsStreamADIOS2_Threads MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadAttributes MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(FStreamWriteReadHighLevelAPI MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteFlushRead MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteMultiblockRead MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadMultiblock MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadVector MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadAttributesMultirank MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(LargeMetadata MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteMemorySelectionRead MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariables MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariablesSel MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariablesSelHighLevel MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(ChangingShape MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadBlockInfo MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadVariableSpan MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(TimeAggregation MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(NoXMLRecovery MPI_ALLOW)

if(NOT MSVC)
  bp3_bp4_gtest_add_tests_helper(BufferSize MPI_NONE)
endif()

if(ADIOS2_HAVE_MPI)
  bp3_bp4_gtest_add_tests_helper(WriteAggregateRead MPI_ONLY)
endif()

# BP3 only for now
gtest_add_tests_helper(WriteNull MPI_ALLOW BP Engine.BP. .BP3
  WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
)

# BP4 only for now
gtest_add_tests_helper(WriteAppendReadADIOS2 MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)

