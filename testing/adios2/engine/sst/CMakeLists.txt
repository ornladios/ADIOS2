#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestSstWrite TestSstWrite.cpp)
add_executable(TestSstRead TestSstRead.cpp)
add_executable(TestSstServer TestSstServer.cpp)
add_executable(TestSstClient TestSstClient.cpp)
add_executable(TestSstParamFails TestSstParamFails.cpp)
if(ADIOS2_HAVE_Fortran)
  add_library(TestData_f OBJECT TestData_mod.f90)

  if (ADIOS2_HAVE_MPI)
    add_executable(TestSstWrite_f TestSstWriteF.f90 $<TARGET_OBJECTS:TestData_f>)
    add_executable(TestSstRead_f TestSstReadF.f90 $<TARGET_OBJECTS:TestData_f>)
  else()
    add_executable(TestSstWrite_f TestSstWriteF_nompi.f90 $<TARGET_OBJECTS:TestData_f>)
    add_executable(TestSstRead_f TestSstReadF_nompi.f90 $<TARGET_OBJECTS:TestData_f>)
  endif()
  target_link_libraries(TestSstWrite_f adios2_f)
  target_link_libraries(TestSstRead_f adios2_f)
endif()

# Workaround for multiple versions of FindSst
if(SST_INCLUDE_DIRS)
  target_include_directories(TestSstWrite PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestSstRead PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestSstServer PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestSstClient PRIVATE ${SST_INCLUDE_DIRS})
endif()
target_link_libraries(TestSstWrite adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestSstRead adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestSstServer adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestSstClient adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestSstParamFails adios2 gtest)

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestSstWrite MPI::MPI_C)
  target_link_libraries(TestSstRead MPI::MPI_C)
  target_link_libraries(TestSstServer MPI::MPI_C)
  target_link_libraries(TestSstClient MPI::MPI_C)
  target_link_libraries(TestSstParamFails MPI::MPI_C)
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

configure_file(
  run_multi_test.in
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_multi_test
  @ONLY
)

add_test(
  NAME ADIOSSstTest.1x1
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 1 -v -p TestSst -arg "MarshalMethod:FFS")
set_tests_properties(ADIOSSstTest.1x1 PROPERTIES TIMEOUT 30 RUN_SERIAL 1)

add_test(
  NAME ADIOSSstTest.1x1BP
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 1 -v -p TestSst -arg "MarshalMethod:BP")
set_tests_properties(ADIOSSstTest.1x1BP PROPERTIES TIMEOUT 30 RUN_SERIAL 1)

add_test(
  NAME ADIOSSstTest.NoReaderNoWait
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 0 -v -p TestSst -arg "RendezvousReaderCount:0,QueueLimit:3,QueueFullPolicy:discard")
set_tests_properties(ADIOSSstTest.NoReaderNoWait PROPERTIES TIMEOUT 30 RUN_SERIAL 1)

if(ADIOS2_HAVE_Fortran)
  add_test(
    NAME ADIOSSstTest.FtoC_1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 1 -v -w TestSstWrite_f -r TestSstRead)
  set_tests_properties(ADIOSSstTest.FtoC_1x1 PROPERTIES TIMEOUT 30  RUN_SERIAL 1) 
  add_test(
    NAME ADIOSSstTest.FtoC_1x1BP
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 1 -v -w TestSstWrite_f -r TestSstRead -arg "MarshalMethod:BP")
  set_tests_properties(ADIOSSstTest.FtoC_1x1BP PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

  add_test(
    NAME ADIOSSstTest.CtoF_1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 1 -v -w TestSstWrite -r TestSstRead_f)
  set_tests_properties(ADIOSSstTest.CtoF_1x1 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

  add_test(
    NAME ADIOSSstTest.FtoF_1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 1 -v -w TestSstWrite_f -r TestSstRead_f)
  set_tests_properties(ADIOSSstTest.FtoF_1x1 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
endif()

if(ADIOS2_HAVE_ZFP)
  add_test(
    NAME ADIOSSstZFPCompression.1x1
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test -rd 5
    -nw 1 -nr 1 -v -p TestSst -arg "CompressionMethod:zfp" )
  set_tests_properties(ADIOSSstZFPCompression.1x1 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
endif()

if (ADIOS2_HAVE_MPI)
  include(${CMAKE_CURRENT_SOURCE_DIR}/TestSSTMxN.cmake)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

add_test(
  NAME ADIOSSstKillReadersTest.Serialized
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_multi_test -test_protocol kill_readers
    -verbose -nw 3 -nr 2 -max_readers 1 -warg 'RendezvousReaderCount:0' -rarg '--ignore_time_gap')
set_tests_properties(ADIOSSstKillReadersTest.Serialized PROPERTIES TIMEOUT 240 RUN_SERIAL 1)

gtest_add_tests(TARGET TestSstParamFails ${extra_test_args})
