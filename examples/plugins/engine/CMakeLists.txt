#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(GenerateExportHeader)
add_library(PluginEngineWrite
  ExampleWritePlugin.cpp
)
target_link_libraries(PluginEngineWrite adios2::cxx11 adios2_core)
generate_export_header(PluginEngineWrite BASE_NAME plugin_engine_write)
target_include_directories(PluginEngineWrite PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>
)

add_library(PluginEngineRead
  ExampleReadPlugin.cpp
)
target_link_libraries(PluginEngineRead adios2::cxx11 adios2_core)
generate_export_header(PluginEngineRead BASE_NAME plugin_engine_read)
target_include_directories(PluginEngineRead PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>
)

add_executable(examplePluginEngine_write
  examplePluginEngine_write.cpp
)
target_link_libraries(examplePluginEngine_write adios2::cxx11)

add_executable(examplePluginEngine_read
  examplePluginEngine_read.cpp
)
target_link_libraries(examplePluginEngine_read adios2::cxx11)
