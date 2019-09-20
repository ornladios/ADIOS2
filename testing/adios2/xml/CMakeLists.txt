#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(XMLConfig ${test_mpi} "" "" "")
target_compile_definitions(Test.XMLConfig PRIVATE
  "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)
