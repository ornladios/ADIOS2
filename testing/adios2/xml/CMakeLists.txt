#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(XMLConfig MPI_ALLOW "" "" "")
gtest_add_tests_helper(XMLConfigSerial MPI_NOEXEC "" "" "")

foreach(tgt
    ${Test.XMLConfig-TARGETS}
    ${Test.XMLConfigSerial-TARGETS}
    )
  target_compile_definitions(${tgt} PRIVATE
    "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    )
endforeach()
