add_library(nlohmann_json INTERFACE)
target_sources(nlohmann_json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/single_include/upstream/nlohmann/json.hpp>
)

target_sources(nlohmann_json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/single_include/nlohmann_json.hpp>
)

target_compile_options(nlohmann_json INTERFACE $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Intel>>:-Wno-all -Wno-error>)

target_include_directories(nlohmann_json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/single_include>
)
install(TARGETS nlohmann_json EXPORT adios2Exports)

