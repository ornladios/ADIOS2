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

include ( TestSupp.cmake )

set (TEST_SET "1x1;NoReaderNoWait;Modes;1x1.Attrs")
set (FORTRAN_TESTS "")
if(ADIOS2_HAVE_Fortran)
  set (FORTRAN_TESTS "FtoC.1x1;CtoF.1x1;FtoF.1x1")
endif()
set (SPECIAL_TESTS "KillReadersSerialized;KillReaders3Max;TimeoutReader;LatestReader;DiscardWriter")

set (MPI_TESTS "")
set (MPI_FORTRAN_TESTS "")
if (ADIOS2_HAVE_MPI)
  set (MPI_TESTS "2x1;1x2;3x5;5x3;DelayedReader_3x5;DelayedReaderBlocking_3x5")
  if (ADIOS_HAVE_Fortran)
    set (MPI_FORTRAN_TESTS "FtoC.3x5;CtoF.3x5;FtoF.3x5")
  endif()
endif()
 
set (ZFP_TESTS "")
if (ADIOS2_HAVE_ZFP)
   set (ZFP_TESTS "ZFPCompression.1x1;ZFPCompression.3x5")
endif()

#
#   Setup tests for SST engine
#
SET (BASIC_SST_TESTS "")
if(ADIOS2_HAVE_SST)
    list (APPEND BASIC_SST_TESTS ${TEST_SET} ${FORTRAN_TESTS} ${SPECIAL_TESTS} ${MPI_TESTS} ${MPI_FORTRAN_TESTS} ${ZFP_TESTS})
endif()


MutateTestSet( COMM_MIN_SST_TESTS "CommMin" "CPCommPattern:Min" "${BASIC_SST_TESTS}" )
MutateTestSet( COMM_PEER_SST_TESTS "CommPeer" "CPCommPattern:Min" "${BASIC_SST_TESTS}" )

MutateTestSet( FFS_SST_TESTS "FFS" "MarshalMethod:FFS" "${COMM_MIN_SST_TESTS};${COMM_PEER_SST_TESTS}" )
MutateTestSet( BP_SST_TESTS "BP" "MarshalMethod:BP" "${COMM_MIN_SST_TESTS};${COMM_PEER_SST_TESTS}" )

set (SST_TESTS "")
LIST (APPEND SST_TESTS ${FFS_SST_TESTS} ${BP_SST_TESTS})

foreach(test ${SST_TESTS})
	     message(Status "ADDING SST TEST ${test}")
    add_common_test(${test} SST)
endforeach()



#
#   Setup tests for WDM engine
#
if(ADIOS2_HAVE_WDM)
SET (WDM_TESTS "NoReaderNoWait;TimeoutReader;LatestReader;DiscardWriter")
foreach(test ${WDM_TESTS})
    add_common_test(${test} WDM)
endforeach()
endif()


