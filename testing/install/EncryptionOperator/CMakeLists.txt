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

# Locate the keygen utility installed alongside ADIOS2
get_filename_component(_adios2_prefix "${adios2_DIR}/../../.." ABSOLUTE)
find_program(ADIOS2_SEAL_KEYGEN adios2_seal_keygen
    HINTS "${_adios2_prefix}/bin"
)

if(ADIOS2_SEAL_KEYGEN)
  add_executable(adios_plugin_sealed_operator_write_test
    ../../../examples/plugins/operator/exampleSealedOperatorWrite.cpp
  )
  target_link_libraries(adios_plugin_sealed_operator_write_test adios2::cxx)

  add_executable(adios_plugin_sealed_operator_read_test
    ../../../examples/plugins/operator/exampleSealedOperatorRead.cpp
  )
  target_link_libraries(adios_plugin_sealed_operator_read_test adios2::cxx)

  # Step 1: generate a Curve25519 keypair
  add_test(NAME adios_plugin_sealed_keygen_test
    COMMAND ${ADIOS2_SEAL_KEYGEN} test-public.key test-secret.key
  )

  # Step 2: write data encrypted with the public key only
  add_test(NAME adios_plugin_sealed_operator_write_test
    COMMAND adios_plugin_sealed_operator_write_test
  )
  set_tests_properties(adios_plugin_sealed_operator_write_test PROPERTIES
    DEPENDS adios_plugin_sealed_keygen_test)

  # Step 3: read and decrypt data using both keys
  add_test(NAME adios_plugin_sealed_operator_read_test
    COMMAND adios_plugin_sealed_operator_read_test
  )
  set_tests_properties(adios_plugin_sealed_operator_read_test PROPERTIES
    DEPENDS adios_plugin_sealed_operator_write_test)
endif()
