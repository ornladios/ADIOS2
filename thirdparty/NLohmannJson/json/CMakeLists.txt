add_library(NLohmannJson INTERFACE)
target_include_directories(NLohmannJson INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/single_include/nlohmann>
)
install(TARGETS NLohmannJson EXPORT adios2Exports)
