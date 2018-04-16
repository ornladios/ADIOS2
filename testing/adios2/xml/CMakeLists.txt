#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestXMLConfig TestXMLConfig.cpp)
target_link_libraries(TestXMLConfig adios2 gtest)
target_compile_definitions(TestXMLConfig PRIVATE
  "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestXMLConfig MPI::MPI_C)

  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestXMLConfig ${extra_test_args})
