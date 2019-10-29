#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(YAMLConfig ${test_mpi} "" "" "")
target_compile_definitions(Test.YAMLConfig PRIVATE
  "YAML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)
