#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Sodium 1.8 REQUIRED)

add_library(EncryptionOperator
  EncryptionOperator.cpp
)
target_link_libraries(EncryptionOperator adios2::cxx11 adios2_core sodium)

add_executable(exampleOperatorPlugin_write
  examplePluginOperator_write.cpp
)
target_link_libraries(exampleOperatorPlugin_write adios2::cxx11)

add_executable(exampleOperatorPlugin_read
  examplePluginOperator_read.cpp
)
target_link_libraries(exampleOperatorPlugin_read adios2::cxx11)
