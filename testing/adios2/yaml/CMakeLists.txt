#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(YAMLConfig MPI_ALLOW "" "" "")
gtest_add_tests_helper(YAMLConfigSerial MPI_NOEXEC "" "" "")

foreach(tgt
    ${Test.YAMLConfig-TARGETS}
    ${Test.YAMLConfigSerial-TARGETS}
    )
  target_compile_definitions(${tgt} PRIVATE
    "YAML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    )
endforeach()
