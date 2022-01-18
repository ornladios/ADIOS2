#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.6)
project(adios_plugin_test CXX)
enable_testing()

find_package(adios2 REQUIRED)

option(BUILD_SHARED_LIBS "build shared libs" ON)

#---------- Engine Plugin Tests

add_library(PluginEngineWrite
  ../../../examples/plugins/engine/ExampleWritePlugin.cpp
)
target_link_libraries(PluginEngineWrite adios2::cxx11 adios2::core)

add_library(PluginEngineRead
  ../../../examples/plugins/engine/ExampleReadPlugin.cpp
)
target_link_libraries(PluginEngineRead adios2::cxx11 adios2::core)

# add write test
add_executable(adios_plugin_engine_write_test
  ../../../examples/plugins/engine/examplePluginEngine_write.cpp
)
target_link_libraries(adios_plugin_engine_write_test adios2::cxx11)
add_test(NAME adios_plugin_engine_write_test COMMAND adios_plugin_engine_write_test)

# add read test
add_executable(adios_plugin_engine_read_test
  ../../../examples/plugins/engine/examplePluginEngine_read.cpp
)
target_link_libraries(adios_plugin_engine_read_test adios2::cxx11)
add_test(NAME adios_plugin_engine_read_test COMMAND adios_plugin_engine_read_test)
set_tests_properties(adios_plugin_engine_read_test PROPERTIES
    DEPENDS adios_plugin_engine_write_test)

#---------- Operator Plugin Tests

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples/plugins/operator)
find_package(Sodium 1.8 REQUIRED)

add_library(EncryptionOperator
    ../../../examples/plugins/operator/EncryptionOperator.cpp
)
target_link_libraries(EncryptionOperator adios2::cxx11 adios2::core sodium)

# add write test
add_executable(adios_plugin_operator_write_test
  ../../../examples/plugins/operator/examplePluginOperator_write.cpp
)
target_link_libraries(adios_plugin_operator_write_test adios2::cxx11)
add_test(NAME adios_plugin_operator_write_test COMMAND adios_plugin_operator_write_test)

# add read test
add_executable(adios_plugin_operator_read_test
  ../../../examples/plugins/operator/examplePluginOperator_read.cpp
)
target_link_libraries(adios_plugin_operator_read_test adios2::cxx11)
add_test(NAME adios_plugin_operator_read_test COMMAND adios_plugin_operator_read_test)
set_tests_properties(adios_plugin_operator_read_test PROPERTIES
    DEPENDS adios_plugin_operator_write_test)
