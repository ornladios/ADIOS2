#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})

if(ADIOS2_HAVE_SZ)
  bp3_bp4_gtest_add_tests_helper(WriteReadSZ MPI_ALLOW)
endif()


if(ADIOS2_HAVE_ZFP)
  bp3_bp4_gtest_add_tests_helper(WriteReadZfp MPI_ALLOW)
  bp3_bp4_gtest_add_tests_helper(WriteReadZfpHighLevelAPI MPI_ALLOW)
  bp3_bp4_gtest_add_tests_helper(WriteReadZfpConfig MPI_ALLOW)

  foreach(tgt
      ${Test.Engine.BP.WriteReadZfpConfig-TARGETS}
      )
    target_compile_definitions(${tgt} PRIVATE
      "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      )
  endforeach()
endif()

if(ADIOS2_HAVE_MGARD)
  bp3_bp4_gtest_add_tests_helper(WriteReadMGARD MPI_ALLOW)
endif()

if(ADIOS2_HAVE_BZip2)
  bp3_bp4_gtest_add_tests_helper(WriteReadBZIP2 MPI_ALLOW)
endif()

if(ADIOS2_HAVE_PNG)
  bp3_bp4_gtest_add_tests_helper(WriteReadPNG MPI_ALLOW)
endif()

if(ADIOS2_HAVE_Blosc)
  bp3_bp4_gtest_add_tests_helper(WriteReadBlosc MPI_ALLOW)
endif()
