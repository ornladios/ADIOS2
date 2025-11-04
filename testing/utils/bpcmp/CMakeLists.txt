add_executable(Test.Utils.BPcmp TestUtilsBPcmp.cpp)

target_link_libraries(Test.Utils.BPcmp adios2::cxx)

add_test(NAME Utils.BPcmp
  COMMAND $<TARGET_FILE:Test.Utils.BPcmp>
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
      -DPython_EXECUTABLE=$<TARGET_FILE:Python::Interpreter>
      -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpcmp.cmake"
  )

if(UNIX)
  set_property(TEST Utils.BPcmp.Stats.Dump PROPERTY
    ENVIRONMENT
      "PYTHONPATH=${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
      "PYTHONUSERBASE=${CMAKE_BINARY_DIR}"
  )
else()
  set_property(TEST Utils.BPcmp.Stats.Dump PROPERTY
    ENVIRONMENT
      "PYTHONPATH=${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR};$<SHELL_PATH:$<TARGET_FILE_DIR:adios2_py>>;$ENV{PYTHONPATH}"
      "PATH=$<SHELL_PATH:$<TARGET_FILE_DIR:adios2_py>>;$<SHELL_PATH:$<TARGET_FILE_DIR:adios2_core>>;$ENV{PATH}"
      "PYTHONUSERBASE=${CMAKE_BINARY_DIR}"
  )
endif()

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
      -DPython_EXECUTABLE=$<TARGET_FILE:Python::Interpreter>
      -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpcmp.cmake"
  )

if(UNIX)
  set_property(TEST Utils.BPcmp.Verbose.Dump PROPERTY
    ENVIRONMENT
      "PYTHONPATH=${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR}:$ENV{PYTHONPATH}"
      "PYTHONUSERBASE=${CMAKE_BINARY_DIR}"
  )
else()
  set_property(TEST Utils.BPcmp.Verbose.Dump PROPERTY
    ENVIRONMENT
      "PYTHONPATH=${ADIOS2_BINARY_DIR}/${CMAKE_INSTALL_PYTHONDIR};$<SHELL_PATH:$<TARGET_FILE_DIR:adios2_py>>;$ENV{PYTHONPATH}"
      "PATH=$<SHELL_PATH:$<TARGET_FILE_DIR:adios2_py>>;$<SHELL_PATH:$<TARGET_FILE_DIR:adios2_core>>;$ENV{PATH}"
      "PYTHONUSERBASE=${CMAKE_BINARY_DIR}"
  )
endif()

add_test(NAME Utils.BPcmp.Verbose.Validate
  COMMAND ${DIFF_COMMAND} -u -w
  ${CMAKE_CURRENT_SOURCE_DIR}/TestUtilsBPcmp.verbose.expected.txt
    TestUtilsBPcmp.verbose.result.txt
)

SetupTestPipeline(Utils.BPcmp ";Verbose.Dump;Verbose.Validate" FALSE)
