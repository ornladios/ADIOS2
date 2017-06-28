add_library(NLohmannJson INTERFACE)
target_include_directories(NLohmannJson INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
install(TARGETS NLohmannJson EXPORT adios2Exports)
