add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/single_include>
)
install(TARGETS nlohmann_json EXPORT adios2Exports)
