#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestXMLConfig TestXMLConfig.cpp)
target_link_libraries(TestXMLConfig adios2 gtest gtest_main)
target_compile_definitions(TestXMLConfig PRIVATE
  "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

gtest_add_tests(TARGET TestXMLConfig)
