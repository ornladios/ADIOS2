cmake_minimum_required(VERSION 3.6)

if(OUTPUT_FILE)
  set(maybe_output_arg OUTPUT_FILE "${OUTPUT_FILE}")
else()
  set(maybe_output_arg)
endif()

if(ERROR_FILE)
  set(maybe_error_arg ERROR_FILE "${ERROR_FILE}")
elseif(NOT INPUT_FILE1)
  set(maybe_error_arg INPUT_FILE1 "${INPUT_FILE1}")
elseif(NOT INPUT_FILE2)
  set(maybe_error_arg INPUT_FILE2 "${INPUT_FILE2}")
else()
  set(maybe_error_arg)
endif()

execute_process(
  COMMAND ${Python_EXECUTABLE} ${PROJECT_BINARY_DIR}/bin/bpcmp.py ${INPUT_FILE1} ${INPUT_FILE2} ${ARG1} ${ARG2} ${ARG3}
  RESULT_VARIABLE result
  ${maybe_output_arg}
  ${maybe_error_arg}
)

if(NOT result EQUAL 0)
  message(FATAL_ERROR "result: ${result}")
endif()
