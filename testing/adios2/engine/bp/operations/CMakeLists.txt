#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})

if(ADIOS2_HAVE_SZ)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadSZ)
endif()


if(ADIOS2_HAVE_ZFP)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadZfp)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadZfpHighLevelAPI)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadZfpConfig)
  target_compile_definitions(Test.Engine.BP.BPWriteReadZfpConfig PRIVATE
    "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
  )
endif()

if(ADIOS2_HAVE_MGARD)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadMGARD)
endif()

if(ADIOS2_HAVE_BZip2)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadBZIP2)
endif()

if(ADIOS2_HAVE_PNG)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadPNG)
endif()

if(ADIOS2_HAVE_Blosc)
  bp3_bp4_gtest_add_tests_helper(BPWriteReadBlosc)
endif()
