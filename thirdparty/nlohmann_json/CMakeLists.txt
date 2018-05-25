message_start_thirdparty()

set(BUILD_TESTING OFF)
set(NLOHMANN_JSON_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_CMAKEDIR}/deps"
  CACHE INTERNAL "")

add_subdirectory(nlohmann_json)

set(nlohmann_json_DIR ${CMAKE_CURRENT_BINARY_DIR}/nlohmann_json
  CACHE INTERNAL "")

message_end_thirdparty()
