message_start_thirdparty()

# third-party library not built with its meson/ninja combo
# but here manually with cmake

add_library(inih INTERFACE)
target_sources(inih INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inih/ini.c>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inih/cpp/INIReader.cpp>
)

target_include_directories(inih INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inih>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inih/cpp>
)

add_definitions(
 -DINI_MAX_LINE=4096
 -DINI_USE_STACK=0
)

message_end_thirdparty()
