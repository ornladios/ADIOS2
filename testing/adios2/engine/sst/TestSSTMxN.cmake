add_test(
  NAME ADIOSSstTest.2x1
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 2 -nr 1 -v -p TestSst)
set_tests_properties(ADIOSSstTest.2x1 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

add_test(
  NAME ADIOSSstTest.1x2
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 1 -nr 2 -v -p TestSst)
set_tests_properties(ADIOSSstTest.1x2 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

add_test(
  NAME ADIOSSstTest.3x5
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 3 -nr 5 -v -p TestSst)
set_tests_properties(ADIOSSstTest.3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

add_test(
  NAME ADIOSSstTest.3x5BP
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 3 -nr 5 -v -p TestSst -arg "MarshalMethod:BP")
set_tests_properties(ADIOSSstTest.3x5BP PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

add_test(
  NAME ADIOSSstTest.5x3
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 5 -nr 3 -v -p TestSst)
set_tests_properties(ADIOSSstTest.5x3 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

if(ADIOS2_HAVE_Fortran)
  add_test(
    NAME ADIOSSstTest.FtoC_3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 3 -nr 5 -v -w TestSstWrite_f -r TestSstRead)
  set_tests_properties(ADIOSSstTest.FtoC_3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

  add_test(
    NAME ADIOSSstTest.FtoC_3x5BP
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 3 -nr 5 -v -w TestSstWrite_f -r TestSstRead -arg "MarshalMethod:BP")
  set_tests_properties(ADIOSSstTest.FtoC_3x5BP PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
  

  add_test(
    NAME ADIOSSstTest.CtoF_3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 3 -nr 5 -v -w TestSstWrite -r TestSstRead_f)
  set_tests_properties(ADIOSSstTest.CtoF_3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
  add_test(
    NAME ADIOSSstTest.FtoF_3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test
    -nw 3 -nr 5 -v -w TestSstWrite_f -r TestSstRead_f)
  set_tests_properties(ADIOSSstTest.FtoF_3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
endif()

add_test(
  NAME ADIOSSstDelayedReaderTest.3x5
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test -rd 5
    -nw 3 -nr 5 -v -p TestSst)
set_tests_properties(ADIOSSstDelayedReaderTest.3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 

add_test(
  NAME ADIOSSstDelayedReaderWithBlockingTest.3x5
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test -rd 5
    -nw 3 -nr 5 -v -p TestSst -arg 'RendezvousReaderCount:0,QueueLimit:3' -arg '--expect_time_gap')
set_tests_properties(ADIOSSstDelayedReaderWithBlockingTest.3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
if(ADIOS2_HAVE_ZFP)
  add_test(
    NAME ADIOSSstZFPCompression.3x5
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_staging_test -rd 5
    -nw 3 -nr 5 -v -p TestSst -arg 'CompressionMethod:zfp' )
  set_tests_properties(ADIOSSstZFPCompression.3x5 PROPERTIES TIMEOUT 30 RUN_SERIAL 1) 
endif()
