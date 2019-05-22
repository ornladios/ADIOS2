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
add_executable(TestCommonWriteLocal TestCommonWriteLocal.cpp)
add_executable(TestCommonRead TestCommonRead.cpp)
add_executable(TestCommonReadAttrs TestCommonReadAttrs.cpp)
add_executable(TestCommonReadLocal TestCommonReadLocal.cpp)
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
  target_include_directories(TestCommonWriteLocal PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonRead PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonReadLocal PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonServer PRIVATE ${SST_INCLUDE_DIRS})
  target_include_directories(TestCommonClient PRIVATE ${SST_INCLUDE_DIRS})
endif()
target_link_libraries(TestCommonWrite adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonWriteModes adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonWriteAttrs adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonWriteLocal adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonRead adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonReadLocal adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonReadAttrs adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonServer adios2 gtest_interface ${Sst_LIBRARY})
target_link_libraries(TestCommonClient adios2 gtest_interface ${Sst_LIBRARY})

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestCommonWrite MPI::MPI_C)
  target_link_libraries(TestCommonWriteModes MPI::MPI_C)
  target_link_libraries(TestCommonWriteAttrs MPI::MPI_C)
  target_link_libraries(TestCommonWriteLocal MPI::MPI_C)
  target_link_libraries(TestCommonRead MPI::MPI_C)
  target_link_libraries(TestCommonReadLocal MPI::MPI_C)
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
  run_test.in
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_test.py
  @ONLY
)

include (TestSupp.cmake )

if (ADIOS2_HAVE_MPI) 
   # we want to know if mpiexec is a shell script or a binary executable
   file(READ ${MPIEXEC} MPIFILEDATA LIMIT 1024 HEX)
   string(LENGTH ${MPIFILEDATA} DATALENGTH)
   math(EXPR last_hex_index "(${DATALENGTH} / 2) - 1")
   foreach(hex_index RANGE ${last_hex_index})
      math (EXPR str_loc "2*${hex_index}")
      string(SUBSTRING "${MPIFILEDATA}" "${str_loc}" "2" char)
      #math (EXPR dec_char "0x${char}" )
      from_hex(${char} dec_char)
      if ((${dec_char} LESS 9) OR (${dec_char} GREATER 126) OR ((${dec_char} GREATER 13) AND (${dec_char} LESS 32)))
          #  message ("$character ${char} is outside of the printable/character ascii range")
	  set (MPIEXEC_IS_BINARY TRUE)
	  break()
      endif()
   endforeach()
endif()

set (TEST_SET "1x1;NoReaderNoWait;Modes;1x1.Attrs;1x1.Local")
set (FORTRAN_TESTS "")
if(ADIOS2_HAVE_Fortran)
  set (FORTRAN_TESTS "FtoC.1x1;CtoF.1x1;FtoF.1x1")
endif()

set (SPECIAL_TESTS "TimeoutReader;LatestReader;DiscardWriter;PreciousTimestep;PreciousTimestepDiscard")
if (MPIEXEC_IS_BINARY)
    # run_test.py can only kill readers/writers if mpiexec is not a shell script
    list(APPEND SPECIAL_TESTS "KillReadersSerialized;KillReaders3Max;KillWriter_2x2;KillWriterTimeout_2x2")
endif()

set (MPI_TESTS "")
set (MPI_FORTRAN_TESTS "")
if (ADIOS2_HAVE_MPI)
  set (MPI_TESTS "2x1;1x2;3x5;5x3;DelayedReader_3x5;2x1.Local;1x2.Local;3x5.Local;5x3.Local;2x1ZeroDataVar")
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


#  For the moment, only test the default comm pattern (Peer)
MutateTestSet( COMM_MIN_SST_TESTS "CommMin" "CPCommPattern:Min" "${BASIC_SST_TESTS}" )
MutateTestSet( COMM_PEER_SST_TESTS "CommPeer" "CPCommPattern:Min" "${BASIC_SST_TESTS}" )

MutateTestSet( FFS_SST_TESTS "FFS" "MarshalMethod:FFS" "${COMM_MIN_SST_TESTS};${COMM_PEER_SST_TESTS}" )
MutateTestSet( BP_SST_TESTS "BP" "MarshalMethod:BP" "${COMM_MIN_SST_TESTS};${COMM_PEER_SST_TESTS}" )

set (SST_TESTS "")
LIST (APPEND SST_TESTS ${FFS_SST_TESTS} ${BP_SST_TESTS})

# remove Fto anything tests that use FFS because we can't spec it
list(FILTER SST_TESTS EXCLUDE REGEX "Fto.*FFS.*")
# remove Fto anything tests that use CommMin because we can't spec it
list(FILTER SST_TESTS EXCLUDE REGEX "Fto.*CommMin.*")

foreach(test ${SST_TESTS})
    add_common_test(${test} SST)
endforeach()

set_tests_properties(Staging.2x1ZeroDataVar.CommMin.BP.SST Staging.2x1ZeroDataVar.CommPeer.BP.SST  PROPERTIES WILL_FAIL TRUE)

#
#   Setup tests for SSC engine
#
if(ADIOS2_HAVE_SSC AND ADIOS2_HAVE_MPI)
    SET (SSC_TESTS "NoReaderNoWait")
    foreach(test ${SSC_TESTS})
        add_common_test(${test} SSC)
    endforeach()
endif()


