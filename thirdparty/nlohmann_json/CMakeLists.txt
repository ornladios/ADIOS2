message_start_thirdparty()

set(BUILD_TESTING OFF)

add_subdirectory(nlohmann_json EXCLUDE_FROM_ALL)
install(TARGETS nlohmann_json EXPORT adios2Exports)

message_end_thirdparty()
