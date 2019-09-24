message_start_thirdparty()

add_library(RapidJSON INTERFACE)
target_include_directories(RapidJSON INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/RapidJSON/include>
)
install(TARGETS RapidJSON EXPORT adios2Exports)

message_end_thirdparty()
