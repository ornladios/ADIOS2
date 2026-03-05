message_start_thirdparty()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nanobind/CMakeLists.txt")
  message(FATAL_ERROR
    "nanobind source is missing in thirdparty/nanobind/nanobind/. "
    "Please run thirdparty/nanobind/update.sh to populate it.")
endif()

add_subdirectory(nanobind)

message_end_thirdparty()
