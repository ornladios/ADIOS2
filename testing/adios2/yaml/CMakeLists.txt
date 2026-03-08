# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(YAMLTimeSeries MPI_NONE "" "" "")
foreach(tgt IN LISTS Test.YAMLTimeSeries-TARGETS)
  target_compile_definitions(${tgt} PRIVATE
    "YAML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
  )
endforeach()
#target_compile_definitions(${Test.YAMLTimeSeries} PRIVATE 
#  "YAML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
#)


gtest_add_tests_helper(YAMLConfig MPI_ALLOW "" "" "")

foreach(tgt IN LISTS Test.YAMLConfig-TARGETS)
  target_compile_definitions(${tgt} PRIVATE
    "YAML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
  )
endforeach()

