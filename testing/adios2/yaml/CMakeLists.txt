#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(YAMLConfig ${test_mpi} "" "" "")
if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(YAMLConfigSerial NOEXEC "" "" "")
else()
  gtest_add_tests_helper(YAMLConfigSerial FALSE "" "" "")
endif()

foreach(tgt
    ${Test.YAMLConfig-TARGETS}
    ${Test.YAMLConfigSerial-TARGETS}
    )
  target_compile_definitions(${tgt} PRIVATE
    "YAML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    )
endforeach()
