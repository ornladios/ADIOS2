#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})

if(ADIOS2_HAVE_MPI)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

if(ADIOS2_HAVE_SZ)
  add_executable(TestBPWriteReadSZ TestBPWriteReadSZ.cpp)
  target_link_libraries(TestBPWriteReadSZ adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadSZ MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadSZ ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadSZ ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
endif()


if(ADIOS2_HAVE_ZFP)
  add_executable(TestBPWriteReadZfp TestBPWriteReadZfp.cpp)
  target_link_libraries(TestBPWriteReadZfp adios2 gtest)
  
  add_executable(TestBPWriteReadZfpHighLevelAPI TestBPWriteReadZfpHighLevelAPI.cpp)
  target_link_libraries(TestBPWriteReadZfpHighLevelAPI adios2 gtest)
  
  add_executable(TestBPWriteReadZfpConfig TestBPWriteReadZfpConfig.cpp)
  target_link_libraries(TestBPWriteReadZfpConfig adios2 gtest)
  target_compile_definitions(TestBPWriteReadZfpConfig PRIVATE
    "XML_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
  )
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadZfp MPI::MPI_C)
    target_link_libraries(TestBPWriteReadZfpConfig MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadZfp ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadZfp ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
  
  gtest_add_tests(TARGET TestBPWriteReadZfpHighLevelAPI ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadZfpHighLevelAPI ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)

  gtest_add_tests(TARGET TestBPWriteReadZfpConfig ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadZfpConfig ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
endif()

if(ADIOS2_HAVE_MGARD)
  add_executable(TestBPWriteReadMGARD TestBPWriteReadMGARD.cpp)
  target_link_libraries(TestBPWriteReadMGARD adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadMGARD MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadMGARD ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadMGARD ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
endif()

if(ADIOS2_HAVE_BZip2)
  add_executable(TestBPWriteReadBZIP2 TestBPWriteReadBZIP2.cpp)
  target_link_libraries(TestBPWriteReadBZIP2 adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadBZIP2 MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadBZIP2 ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadBZIP2 ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
endif()

if(ADIOS2_HAVE_PNG)
  add_executable(TestBPWriteReadPNG TestBPWriteReadPNG.cpp)
  target_link_libraries(TestBPWriteReadPNG adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadPNG MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadPNG ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadPNG ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
endif()

if(ADIOS2_HAVE_Blosc)
  add_executable(TestBPWriteReadBlosc TestBPWriteReadBlosc.cpp)
  target_link_libraries(TestBPWriteReadBlosc adios2 gtest)
  
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestBPWriteReadBlosc MPI::MPI_C)
  endif()
  
  gtest_add_tests(TARGET TestBPWriteReadBlosc ${extra_test_args} WORKING_DIRECTORY ${BP3_DIR})
  gtest_add_tests(TARGET TestBPWriteReadBlosc ${extra_test_args} WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4" TEST_SUFFIX _BP4)
endif()
