#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_library(PluginEngineWriteExample
  ExampleWritePlugin.cpp
)
target_link_libraries(PluginEngineWriteExample adios2::cxx11 adios2_core)

add_library(PluginEngineReadExample
  ExampleReadPlugin.cpp
)
target_link_libraries(PluginEngineReadExample adios2::cxx11 adios2_core)

add_executable(examplePluginEngine_write
  examplePluginEngine_write.cpp
)
target_link_libraries(examplePluginEngine_write adios2::cxx11)

add_executable(examplePluginEngine_read
  examplePluginEngine_read.cpp
)
target_link_libraries(examplePluginEngine_read adios2::cxx11)
