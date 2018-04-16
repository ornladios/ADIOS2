#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestSstWrite TestSstWrite.cpp)
add_executable(TestSstRead TestSstRead.cpp)

# Workaround for multiple versions of FindSst
if(SST_INCLUDE_DIRS)
  target_include_directories(TestSstWrite PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestSstRead PRIVATE ${SST_INCLUDE_DIRS})
endif()
target_link_libraries(TestSstWrite adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestSstRead adios2 gtest_interface ${Sst_LIBRARY})

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestSstWrite MPI::MPI_C)
  target_link_libraries(TestSstRead MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

configure_file(
  run_staging_test.in
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
  @ONLY
)

add_test(
  NAME ADIOSSstTest.Simple_1x1
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 1 -nw 1 -v -p TestSst
)
set_tests_properties(ADIOSSstTest.Simple_1x1 PROPERTIES TIMEOUT 60)
