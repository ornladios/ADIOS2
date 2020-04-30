#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(XMLConfig ${test_mpi} "" "" "")
if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(XMLConfigSerial NOEXEC "" "" "")
else()
  gtest_add_tests_helper(XMLConfigSerial FALSE "" "" "")
endif()

foreach(tgt
    ${Test.XMLConfig-TARGETS}
    ${Test.XMLConfigSerial-TARGETS}
    )
  target_compile_definitions(${tgt} PRIVATE
    "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    )
endforeach()
