#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
add_executable(exampleOperatorPlugin_write
  examplePluginOperator_write.cpp
)
target_link_libraries(exampleOperatorPlugin_write adios2::cxx11)

add_executable(exampleOperatorPlugin_read
  examplePluginOperator_read.cpp
)
target_link_libraries(exampleOperatorPlugin_read adios2::cxx11)
