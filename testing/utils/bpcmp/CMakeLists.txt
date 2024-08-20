include(ADIOSFunctions)

add_executable(Test.Utils.BPcmp TestUtilsBPcmp.cpp)

target_link_libraries(Test.Utils.BPcmp adios2::c adios2::cxx11)
set(cmd_executor)

add_test(NAME Utils.BPcmp
  COMMAND ${cmd_executor} $<TARGET_FILE:Test.Utils.BPcmp>
)
# This test produces TestBPcmp_set1.bp and TestBPcmp_set2.bp

########################################
# bpcmp --stats
########################################
add_test(NAME Utils.BPcmp.Stats.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=--stats
    -DINPUT_FILE1=TestBPcmp_set1.bp
    -DINPUT_FILE2=TestBPcmp_set2.bp
    -DOUTPUT_FILE=TestUtilsBPcmp.stats.result.txt
    -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
    -DPython_EXECUTABLE=${Python_EXECUTABLE}
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpcmp.cmake"
)

add_test(NAME Utils.BPcmp.Stats.Validate
  COMMAND ${DIFF_COMMAND} -u -w
  ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsBPcmp.stats.expected.txt
    TestUtilsBPcmp.stats.result.txt
)

SetupTestPipeline(Utils.BPcmp ";Stats.Dump;Stats.Validate" TRUE)

########################################
# bpcmp --verbose
########################################
add_test(NAME Utils.BPcmp.Verbose.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=--verbose
    -DINPUT_FILE1=TestBPcmp_set1.bp
    -DINPUT_FILE2=TestBPcmp_set2.bp
    -DOUTPUT_FILE=TestUtilsBPcmp.verbose.result.txt
    -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
    -DPython_EXECUTABLE=${Python_EXECUTABLE}
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpcmp.cmake"
)

add_test(NAME Utils.BPcmp.Verbose.Validate
  COMMAND ${DIFF_COMMAND} -u -w
  ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsBPcmp.verbose.expected.txt
    TestUtilsBPcmp.verbose.result.txt
)

SetupTestPipeline(Utils.BPcmp ";Verbose.Dump;Verbose.Validate" FALSE)
