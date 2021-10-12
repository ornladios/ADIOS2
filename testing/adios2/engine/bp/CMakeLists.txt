#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
set(BP5_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp5)
set(FS_DIR ${CMAKE_CURRENT_BINARY_DIR}/filestream)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})
file(MAKE_DIRECTORY ${BP5_DIR})
file(MAKE_DIRECTORY ${FS_DIR})

macro(bp3_bp4_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP3
    WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
endmacro()

macro(bp_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP3
    WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
  if(ADIOS2_HAVE_BP5)
    gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP5
      WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
    )
  endif()
endmacro()

add_subdirectory(operations)

bp_gtest_add_tests_helper(WriteReadADIOS2 MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadADIOS2fstream MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadADIOS2stdio MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAsStreamADIOS2 MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAsStreamADIOS2_Threads MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAttributes MPI_ALLOW)
bp_gtest_add_tests_helper(FStreamWriteReadHighLevelAPI MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteFlushRead MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteMultiblockRead MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadMultiblock MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadVector MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadAttributesMultirank MPI_ALLOW)
bp_gtest_add_tests_helper(LargeMetadata MPI_ALLOW)
bp_gtest_add_tests_helper(WriteMemorySelectionRead MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadLocalVariables MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadLocalVariablesSel MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadLocalVariablesSelHighLevel MPI_ALLOW)
bp_gtest_add_tests_helper(ChangingShape MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(WriteReadBlockInfo MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadVariableSpan MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(TimeAggregation MPI_ALLOW)
bp_gtest_add_tests_helper(NoXMLRecovery MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(StepsFileGlobalArray MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(StepsFileLocalArray MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(SelectSteps MPI_ALLOW)

bp3_bp4_gtest_add_tests_helper(SelectionsOnRowMajorData MPI_NONE)
bp3_bp4_gtest_add_tests_helper(SelectionsOnColumnMajorData MPI_NONE)

if(NOT MSVC)
  bp3_bp4_gtest_add_tests_helper(BufferSize MPI_NONE)
endif()

if(ADIOS2_HAVE_MPI)
  bp_gtest_add_tests_helper(WriteAggregateRead MPI_ONLY)
endif()

# BP3 only for now
gtest_add_tests_helper(WriteNull MPI_ALLOW BP Engine.BP. .BP3
  WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
)

# BP4 only for now
gtest_add_tests_helper(WriteAppendReadADIOS2 MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
gtest_add_tests_helper(StepsInSituGlobalArray MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
gtest_add_tests_helper(StepsInSituLocalArray MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)

gtest_add_tests_helper(InquireVariableException MPI_ALLOW BP Engine.BP. .BP4
        WORKING_DIRECTORY ${BP4_DIR}
)
gtest_add_tests_helper(InquireDefine MPI_ALLOW BP Engine.BP. .BP4
        WORKING_DIRECTORY ${BP4_DIR}
)

# FileStream is BP4 + StreamReader=true
gtest_add_tests_helper(StepsInSituGlobalArray MPI_ALLOW BP Engine.BP. .FileStream
  WORKING_DIRECTORY ${FS_DIR} EXTRA_ARGS "FileStream"
)
gtest_add_tests_helper(StepsInSituLocalArray MPI_ALLOW BP Engine.BP. .FileStream
  WORKING_DIRECTORY ${FS_DIR} EXTRA_ARGS "FileStream"
)

