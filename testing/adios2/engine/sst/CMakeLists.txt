#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestSstWrite TestSstWrite.cpp)
add_executable(TestSstRead TestSstRead.cpp)
if(ADIOS2_HAVE_Fortran)
  add_library(TestData_f OBJECT TestData_mod.f90)
  add_executable(TestSstWrite_f TestSstWriteF.f90 $<TARGET_OBJECTS:TestData_f>)
  target_link_libraries(TestSstWrite_f adios2_f)
  add_executable(TestSstRead_f TestSstReadF.f90 $<TARGET_OBJECTS:TestData_f>)
  target_link_libraries(TestSstRead_f adios2_f)
endif()

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
  if(ADIOS2_HAVE_Fortran)
    target_link_libraries(TestSstWrite_f MPI::MPI_Fortran)
    target_link_libraries(TestSstRead_f MPI::MPI_Fortran)
  endif()
endif()

configure_file(
  run_staging_test.in
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
  @ONLY
)

add_test(
  NAME ADIOSSstTest.Simple_1x1
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 1 -nw 1 -v -p TestSst)

add_test(
  NAME ADIOSSstTest.2x1
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 2 -nw 1 -v -p TestSst)

add_test(
  NAME ADIOSSstTest.1x2
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 1 -nw 2 -v -p TestSst)

add_test(
  NAME ADIOSSstTest.3x5
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 3 -nw 5 -v -p TestSst)

add_test(
  NAME ADIOSSstTest.5x3
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 5 -nw 3 -v -p TestSst)

if(ADIOS2_HAVE_Fortran)
  add_test(
    NAME ADIOSSstTest.FtoC_1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 1 -nw 1 -v -w TestSstWrite_f -r TestSstRead)
  add_test(
    NAME ADIOSSstTest.CtoF_1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 1 -nw 1 -v -w TestSstWrite -r TestSstRead_f)
  add_test(
    NAME ADIOSSstTest.FtoF_1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 1 -nw 1 -v -w TestSstWrite_f -r TestSstRead_f)
  add_test(
    NAME ADIOSSstTest.FtoC_3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 3 -nw 5 -v -w TestSstWrite_f -r TestSstRead)
  add_test(
    NAME ADIOSSstTest.CtoF_3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 3 -nw 5 -v -w TestSstWrite -r TestSstRead_f)
  add_test(
    NAME ADIOSSstTest.FtoF_3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nr 3 -nw 5 -v -w TestSstWrite_f -r TestSstRead_f)
endif()
