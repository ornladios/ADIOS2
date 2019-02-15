#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(Threads REQUIRED)

if(ADIOS2_HAVE_MPI)
    set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})

    add_executable(TestStagingMPMD  TestStagingMPMD.cpp)
    target_link_libraries(TestStagingMPMD adios2 gtest ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(TestStagingMPMD adios2 gtest_interface MPI::MPI_C)
    if(ADIOS2_HAVE_SST)
        gtest_add_tests(TARGET TestStagingMPMD ${extra_test_args} 
        		EXTRA_ARGS "SST" "MarshalMethod:FFS"
			TEST_SUFFIX _SST_FFS)
	gtest_add_tests(TARGET TestStagingMPMD ${extra_test_args} 
        	        EXTRA_ARGS "SST" "MarshalMethod:BP"
                	TEST_SUFFIX _SST_BP)
    endif()

    gtest_add_tests(TARGET TestStagingMPMD ${extra_test_args} 
                    EXTRA_ARGS "InSituMPI"
                    TEST_SUFFIX _InSituMPI)
endif()

add_executable(TestCommonWrite TestCommonWrite.cpp)
add_executable(TestCommonWriteAttrs TestCommonWriteAttrs.cpp)
add_executable(TestCommonWriteModes TestCommonWriteModes.cpp)
add_executable(TestCommonRead TestCommonRead.cpp)
add_executable(TestCommonReadAttrs TestCommonReadAttrs.cpp)
add_executable(TestCommonServer TestCommonServer.cpp)
add_executable(TestCommonClient TestCommonClient.cpp)
if(ADIOS2_HAVE_Fortran)
  add_library(TestCommonData_f OBJECT TestData_mod.F90)

  add_executable(TestCommonWrite_f TestCommonWriteF.F90 $<TARGET_OBJECTS:TestCommonData_f>)
  add_executable(TestCommonRead_f TestCommonReadF.F90 $<TARGET_OBJECTS:TestCommonData_f>)
  target_link_libraries(TestCommonWrite_f adios2_f)
  target_link_libraries(TestCommonRead_f adios2_f)
endif()

# Workaround for multiple versions of FindSst
if(SST_INCLUDE_DIRS)
  target_include_directories(TestCommonWrite PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonWriteModes PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonWriteAttrs PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonRead PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonServer PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonClient PRIVATE ${SST_INCLUDE_DIRS})
endif()
target_link_libraries(TestCommonWrite adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonWriteModes adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonWriteAttrs adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonRead adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonReadAttrs adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonServer adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonClient adios2 gtest_interface ${Sst_LIBRARY})

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestCommonWrite MPI::MPI_C)
  target_link_libraries(TestCommonWriteModes MPI::MPI_C)
  target_link_libraries(TestCommonWriteAttrs MPI::MPI_C)
  target_link_libraries(TestCommonRead MPI::MPI_C)
  target_link_libraries(TestCommonReadAttrs MPI::MPI_C)
  target_link_libraries(TestCommonServer MPI::MPI_C)
  target_link_libraries(TestCommonClient MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
  if(ADIOS2_HAVE_Fortran)
    target_compile_definitions(TestCommonWrite_f PRIVATE ADIOS2_HAVE_MPI_F=1)
    target_compile_definitions(TestCommonRead_f PRIVATE ADIOS2_HAVE_MPI_F=1)
    target_link_libraries(TestCommonWrite_f MPI::MPI_Fortran)
    target_link_libraries(TestCommonRead_f MPI::MPI_Fortran)
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


set (TEST_SET "1x1.FFS;1x1.BP;NoReaderNoWait;Modes;1x1.Attrs")
set (FORTRAN_TESTS "")
if(ADIOS2_HAVE_Fortran)
  set (FORTRAN_TESTS "FtoC.1x1;CtoF.1x1.FFS;CtoF.1x1.BP;FtoF.1x1")
endif()
#set (SPECIAL_TESTS "KillReadersSerialized;KillReaders3Max;TimeoutReader;LatestReader.FFS;LatestReader.BP;DiscardWriter")
set (SPECIAL_TESTS "TimeoutReader;LatestReader.FFS;LatestReader.BP;DiscardWriter")

set (MPI_TESTS "")
set (MPI_FORTRAN_TESTS "")
if (ADIOS2_HAVE_MPI)
  set (MPI_TESTS "2x1;1x2;3x5;5x3;DelayedReader.3x5;DelayedReaderBlocking.3x5")
  if (ADIOS_HAVE_Fortran)
    set (MPI_FORTRAN_TESTS "FtoC.3x5;CtoF.3x5.FFS;CtoF.3x5.BP;FtoF.3x5")
  endif()
endif()
 
set (ZFP_TESTS "")
if (ADIOS2_HAVE_ZFP)
   set (ZFP_TESTS "ZFPCompression.1x1;ZFPCompression.3x5")
endif()

set (1x1.FFS_CMD "run_staging_test -nw 1 -nr 1 -v -p TestCommon -arg MarshalMethod:FFS")
set (1x1.BP_CMD "run_staging_test -nw 1 -nr 1 -v -p TestCommon -arg MarshalMethod:BP")
set (2x1_CMD "run_staging_test -nw 2 -nr 1 -v -p TestCommon")
set (1x2_CMD "run_staging_test -nw 1 -nr 2 -v -p TestCommon")
set (3x5_CMD "run_staging_test -nw 3 -nr 5 -v -p TestCommon")
set (5x3_CMD "run_staging_test -nw 5 -nr 3 -v -p TestCommon")
set (DelayedReader.3x5_CMD "run_staging_test -rd 5 -nw 3 -nr 5 -p TestCommon")
set (DelayedReaderBlocking.3x5_CMD "run_staging_test -rd 5 -nw 3 -nr 5 -v -p TestCommon -arg RendezvousReaderCount:0,QueueLimit:3 -arg --expect_time_gap")
set (FtoC.3x5_CMD "run_staging_test -nw 3 -nr 5 -v -w TestCommonWrite_f -r TestCommonRead -arg MarshalMethod:FFS")
set (CtoF.5x3.BP_CMD "run_staging_test -nw 5 -nr 3 -v -w TestCommonWrite -r TestCommonRead_f -arg MarshalMethod:BP")
set (CtoF.5x3.FFS_CMD "run_staging_test -nw 5 -nr 3 -v -w TestCommonWrite -r TestCommonRead_f -arg MarshalMethod:FFS")
set (FtoF.3x5_CMD "run_staging_test -nw 3 -nr 5 -v -w TestCommonWrite_f -r TestCommonRead_f")

set (NoReaderNoWait_CMD "run_staging_test -nw 1 -nr 0 -v -p TestCommon -arg RendezvousReaderCount:0,QueueLimit:3,QueueFullPolicy:discard")
set (Modes_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWriteModes -r TestCommonRead")
set (1x1.Attrs_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWriteAttrs -r TestCommonReadAttrs")
set (FtoC.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite_f -r TestCommonRead -arg MarshalMethod:FFS")
set (CtoF.1x1.FFS_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite -r TestCommonRead_f -arg MarshalMethod:FFS")
set (CtoF.1x1.BP_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite -r TestCommonRead_f -arg MarshalMethod:BP")
set (FtoF.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite_f -r TestCommonRead_f")
set (ZFPCompression.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -p TestCommon -arg CompressionMethod:zfp" )
set (ZFPCompression.3x5_CMD "run_staging_test -nw 3 -nr 5 -v -p TestCommon -arg CompressionMethod:zfp" )
set (KillReadersSerialized_CMD "run_multi_test -test_protocol kill_readers  -verbose -nw 3 -nr 2 -max_readers 1 -warg RendezvousReaderCount:0,ControlTransport:sockets -rarg --ignore_time_gap")
set (KillReadersSerialized_TIMEOUT "300")
set (KillReaders3Max_CMD "run_multi_test -test_protocol kill_readers  -verbose -nw 3 -nr 2 -max_readers 3 -warg RendezvousReaderCount:0,ControlTransport:sockets -rarg --ignore_time_gap")
set (KillReaders3Max_TIMEOUT "300")
set (TimeoutReader_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -max_readers 1 -rarg --non_blocking -warg --ms_delay -warg 2000")
set (LatestReader.FFS_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -max_readers 1 -warg --ms_delay -warg 250 -warg --engine_params -warg MarshalMethod:FFS -rarg --latest -rarg --long_first_delay")
set (LatestReader.BP_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -max_readers 1 -warg --ms_delay -warg 250 -warg --engine_params -warg MarshalMethod:BP -rarg --latest -rarg --long_first_delay")
set (DiscardWriter_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -max_readers 1 -warg --engine_params -warg QueueLimit:1,QueueFullPolicy:discard -warg --ms_delay -warg 500 -rarg --discard")

function(add_common_test basename engine)
    set(testname "Staging.${basename}.${engine}")
    set(filename "Staging.${basename}.${engine}")
    if ("${${basename}_CMD}" STREQUAL "") 
       message(SEND_ERROR "Staging-Common test ${basename} has no defined ${basename}_CMD")
    endif()
    string (CONCAT command "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/" ${${basename}_CMD})
    separate_arguments(command)
    list(INSERT command 1 "-e" "${engine}" "-f" "${filename}")
    add_test(
    	NAME ${testname}
	COMMAND ${command})
    set (timeout "${${basename}_TIMEOUT}")
    if ("${timeout}" STREQUAL "")
       set (timeout "30")
    endif()

    set_tests_properties(${testname} PROPERTIES TIMEOUT ${timeout})
endfunction()


#
#   Setup tests for SST engine
#
SET (SST_TESTS "")
if(ADIOS2_HAVE_SST)
    list (APPEND SST_TESTS ${TEST_SET} ${FORTRAN_TESTS} ${SPECIAL_TESTS} ${MPI_TESTS} ${MPI_FORTRAN_TESTS} ${ZFP_TESTS})
endif()
foreach(test ${SST_TESTS})
    add_common_test(${test} SST)
endforeach()

