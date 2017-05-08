add_library(NLohmannJson INTERFACE)
target_include_directories(NLohmannJson
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/src
)
