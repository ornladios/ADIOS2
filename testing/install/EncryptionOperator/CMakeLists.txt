#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.6)
project(adios_operator_plugin_test CXX)
enable_testing()
set(CMAKE_CXX_STANDARD 14)

find_package(adios2 REQUIRED)

option(BUILD_SHARED_LIBS "build shared libs" ON)

set(ENV{ADIOS2_PLUGIN_PATH} "${adios2_DIR}/../../")

#---------- Symmetric Operator Plugin Tests

# add write test
add_executable(adios_plugin_operator_write_test
  ../../../examples/plugins/operator/examplePluginOperatorWrite.cpp
)
target_link_libraries(adios_plugin_operator_write_test adios2::cxx)
add_test(NAME adios_plugin_operator_write_test COMMAND adios_plugin_operator_write_test)

# add read test
add_executable(adios_plugin_operator_read_test
  ../../../examples/plugins/operator/examplePluginOperatorRead.cpp
)
target_link_libraries(adios_plugin_operator_read_test adios2::cxx)
add_test(NAME adios_plugin_operator_read_test COMMAND adios_plugin_operator_read_test)
set_tests_properties(adios_plugin_operator_read_test PROPERTIES
    DEPENDS adios_plugin_operator_write_test)

#---------- Asymmetric Operator Plugin Tests

add_executable(adios_plugin_sealed_operator_write_test
  ../../../examples/plugins/operator/exampleSealedOperatorWrite.cpp
)
target_link_libraries(adios_plugin_sealed_operator_write_test adios2::cxx)

add_executable(adios_plugin_sealed_operator_read_test
  ../../../examples/plugins/operator/exampleSealedOperatorRead.cpp
)
target_link_libraries(adios_plugin_sealed_operator_read_test adios2::cxx)

add_test(NAME adios_plugin_sealed_keygen_fixture
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/test-public.key
    ${CMAKE_CURRENT_SOURCE_DIR}/test-secret.key
    .
)

add_test(NAME adios_plugin_sealed_operator_write_test
  COMMAND adios_plugin_sealed_operator_write_test
)
set_tests_properties(adios_plugin_sealed_operator_write_test PROPERTIES
  DEPENDS adios_plugin_sealed_keygen_fixture)

add_test(NAME adios_plugin_sealed_operator_read_test
  COMMAND adios_plugin_sealed_operator_read_test
)
set_tests_properties(adios_plugin_sealed_operator_read_test PROPERTIES
  DEPENDS adios_plugin_sealed_operator_write_test)

# Step 4: negative tests — various wrong usages must throw
add_executable(adios_plugin_sealed_operator_wrong_usage_test
  TestSealedOperatorWrongUsage.cpp
)
target_link_libraries(adios_plugin_sealed_operator_wrong_usage_test adios2::cxx)
add_test(NAME adios_plugin_sealed_operator_wrong_usage_test
  COMMAND adios_plugin_sealed_operator_wrong_usage_test
)
set_tests_properties(adios_plugin_sealed_operator_wrong_usage_test PROPERTIES
  DEPENDS adios_plugin_sealed_keygen_fixture)
