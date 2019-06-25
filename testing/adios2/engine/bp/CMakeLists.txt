#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})

add_subdirectory(operations)

add_executable(TestBPWriteReadADIOS2 TestBPWriteReadADIOS2.cpp)
target_link_libraries(TestBPWriteReadADIOS2 adios2 gtest)

add_executable(TestBPWriteReadADIOS2fstream TestBPWriteReadADIOS2fstream.cpp)
target_link_libraries(TestBPWriteReadADIOS2fstream adios2 gtest)

add_executable(TestBPWriteReadADIOS2stdio TestBPWriteReadADIOS2stdio.cpp)
target_link_libraries(TestBPWriteReadADIOS2stdio adios2 gtest)

add_executable(TestBPWriteReadAsStreamADIOS2 TestBPWriteReadAsStreamADIOS2.cpp)
target_link_libraries(TestBPWriteReadAsStreamADIOS2 adios2 gtest)

add_executable(TestBPWriteReadAsStreamADIOS2_Threads
  TestBPWriteReadAsStreamADIOS2_Threads.cpp
)
target_link_libraries(TestBPWriteReadAsStreamADIOS2_Threads adios2 gtest)

add_executable(TestBPWriteReadAttributesADIOS2
  TestBPWriteReadAttributesADIOS2.cpp
)
target_link_libraries(TestBPWriteReadAttributesADIOS2 adios2 gtest)

add_executable(TestStreamWriteReadHighLevelAPI
  TestStreamWriteReadHighLevelAPI.cpp
)
target_link_libraries(TestStreamWriteReadHighLevelAPI adios2 gtest)

add_executable(TestBPWriteFlushRead TestBPWriteFlushRead.cpp)
target_link_libraries(TestBPWriteFlushRead adios2 gtest)

add_executable(TestBPWriteMultiblockRead TestBPWriteMultiblockRead.cpp)
target_link_libraries(TestBPWriteMultiblockRead adios2 gtest)

add_executable(TestBPWriteReadMultiblock TestBPWriteReadMultiblock.cpp)
target_link_libraries(TestBPWriteReadMultiblock adios2 gtest)

add_executable(TestBPWriteReadVector TestBPWriteReadVector.cpp)
target_link_libraries(TestBPWriteReadVector adios2 gtest)

add_executable(TestBPWriteReadAttributesMultirank TestBPWriteReadAttributesMultirank.cpp)
target_link_libraries(TestBPWriteReadAttributesMultirank adios2 gtest)

add_executable(TestBPLargeMetadata TestBPLargeMetadata.cpp)
target_link_libraries(TestBPLargeMetadata adios2 gtest)

add_executable(TestBPWriteMemorySelectionRead TestBPWriteMemorySelectionRead.cpp)
target_link_libraries(TestBPWriteMemorySelectionRead adios2 gtest)

add_executable(TestBPWriteReadLocalVariables TestBPWriteReadLocalVariables.cpp)
target_link_libraries(TestBPWriteReadLocalVariables adios2 gtest)

add_executable(TestBPWriteReadLocalVariablesSel TestBPWriteReadLocalVariablesSel.cpp)
target_link_libraries(TestBPWriteReadLocalVariablesSel adios2 gtest)

add_executable(TestBPWriteReadLocalVariablesSelHighLevel TestBPWriteReadLocalVariablesSelHighLevel.cpp)
target_link_libraries(TestBPWriteReadLocalVariablesSelHighLevel adios2 gtest)

add_executable(TestBPChangingShape TestBPChangingShape.cpp)
target_link_libraries(TestBPChangingShape adios2 gtest)

add_executable(TestBPWriteReadBlockInfo TestBPWriteReadBlockInfo.cpp)
target_link_libraries(TestBPWriteReadBlockInfo adios2 gtest)

add_executable(TestBPWriteReadVariableSpan TestBPWriteReadVariableSpan.cpp)
target_link_libraries(TestBPWriteReadVariableSpan adios2 gtest)

add_executable(TestBPWriteAppendReadADIOS2 TestBPWriteAppendReadADIOS2.cpp)
target_link_libraries(TestBPWriteAppendReadADIOS2 adios2 gtest)

add_executable(TestBPWriteNull TestBPWriteNull.cpp)
target_link_libraries(TestBPWriteNull adios2 gtest)

if(ADIOS2_HAVE_MPI)

  target_link_libraries(TestBPWriteReadADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteReadADIOS2fstream MPI::MPI_C)
  target_link_libraries(TestBPWriteReadADIOS2stdio MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAsStreamADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAsStreamADIOS2_Threads MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAttributesADIOS2 MPI::MPI_C)
  target_link_libraries(TestStreamWriteReadHighLevelAPI MPI::MPI_C)
  target_link_libraries(TestBPWriteFlushRead MPI::MPI_C)
  target_link_libraries(TestBPWriteMultiblockRead MPI::MPI_C)
  target_link_libraries(TestBPWriteReadMultiblock MPI::MPI_C)
  target_link_libraries(TestBPWriteReadVector MPI::MPI_C)
  target_link_libraries(TestBPWriteReadAttributesMultirank MPI::MPI_C)
  target_link_libraries(TestBPLargeMetadata MPI::MPI_C)
  target_link_libraries(TestBPWriteMemorySelectionRead MPI::MPI_C)
  target_link_libraries(TestBPWriteReadLocalVariables MPI::MPI_C)
  target_link_libraries(TestBPWriteReadLocalVariablesSel MPI::MPI_C)
  target_link_libraries(TestBPWriteReadLocalVariablesSelHighLevel MPI::MPI_C)
  target_link_libraries(TestBPChangingShape MPI::MPI_C)
  target_link_libraries(TestBPWriteReadBlockInfo MPI::MPI_C)
  target_link_libraries(TestBPWriteReadVariableSpan MPI::MPI_C)
  target_link_libraries(TestBPWriteAppendReadADIOS2 MPI::MPI_C)
  target_link_libraries(TestBPWriteNull MPI::MPI_C)
  
  add_executable(TestBPWriteAggregateRead TestBPWriteAggregateRead.cpp)
  target_link_libraries(TestBPWriteAggregateRead
    adios2 gtest_interface MPI::MPI_C
  )
  
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
  gtest_add_tests(TARGET TestBPWriteAggregateRead ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteAggregateRead ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
  
endif()

gtest_add_tests(TARGET TestBPWriteReadADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadADIOS2fstream ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadADIOS2stdio ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2_Threads ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadAttributesADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestStreamWriteReadHighLevelAPI ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteFlushRead ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteMultiblockRead ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadMultiblock ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadVector ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadAttributesMultirank ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPLargeMetadata ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteMemorySelectionRead ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadLocalVariables ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadLocalVariablesSel ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadLocalVariablesSelHighLevel ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPChangingShape ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})

gtest_add_tests(TARGET TestBPWriteReadADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadADIOS2fstream ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadADIOS2stdio ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadAsStreamADIOS2_Threads ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadAttributesADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestStreamWriteReadHighLevelAPI ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteFlushRead ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteMultiblockRead ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadMultiblock ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadVector ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadAttributesMultirank ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPLargeMetadata ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteMemorySelectionRead ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadLocalVariables ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadLocalVariablesSel ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPWriteReadLocalVariablesSelHighLevel ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
gtest_add_tests(TARGET TestBPChangingShape ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)

# BP3 only for now
gtest_add_tests(TARGET TestBPWriteReadBlockInfo ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteReadVariableSpan ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
gtest_add_tests(TARGET TestBPWriteNull ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})

# BP4 only for now
gtest_add_tests(TARGET TestBPWriteAppendReadADIOS2 ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR})
