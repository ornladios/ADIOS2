#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(Threads REQUIRED)

gtest_add_tests_helper(StagingMPMD MPI_ONLY "" Engine.Staging. ".InSituMPI" EXTRA_ARGS "InSituMPI")
if(ADIOS2_HAVE_SST)
  gtest_add_tests_helper(StagingMPMD MPI_ONLY "" Engine.Staging. ".SST.FFS" EXTRA_ARGS "SST" "MarshalMethod=FFS")
  gtest_add_tests_helper(StagingMPMD MPI_ONLY "" Engine.Staging. ".SST.BP" EXTRA_ARGS "SST" "MarshalMethod=BP")
  gtest_add_tests_helper(Threads MPI_NONE "" Engine.Staging. ".SST.FFS" EXTRA_ARGS "SST"  "MarshalMethod=FFS")
  gtest_add_tests_helper(Threads MPI_NONE "" Engine.Staging. ".SST.BP" EXTRA_ARGS "SST"  "MarshalMethod=BP")
  gtest_add_tests_helper(Threads MPI_NONE "" Engine.Staging. ".BP4_stream" EXTRA_ARGS "BP4"  "OpenTimeoutSecs=5")
endif()

foreach(helper
    TestCommonWrite
    TestCommonWriteModes
    TestCommonWriteAttrs
    TestCommonWriteLocal
    TestCommonWriteShared
    TestCommonRead
    TestCommonReadR64
    TestCommonReadLocal
    TestCommonReadShared
    TestCommonReadAttrs
    TestCommonServer
    TestCommonClient
    )
  add_executable(${helper} ${helper}.cpp)

  # Workaround for multiple versions of FindSst
  if(SST_INCLUDE_DIRS)
    target_include_directories(${helper} PRIVATE ${SST_INCLUDE_DIRS})
  endif()

  target_link_libraries(${helper} adios2::thirdparty::gtest ${Sst_LIBRARY})
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(${helper} adios2::cxx11_mpi adios2_core_mpi MPI::MPI_C)
  else()
    target_link_libraries(${helper} adios2::cxx11 adios2_core)
  endif()
endforeach()

if(ADIOS2_HAVE_Fortran)
  add_library(TestCommonData_f OBJECT TestData_mod.F90)
  add_executable(TestCommonWrite_f
    TestCommonWriteF.F90
    $<TARGET_OBJECTS:TestCommonData_f>
  )
  add_executable(TestCommonRead_f
    TestCommonReadF.F90
    $<TARGET_OBJECTS:TestCommonData_f>
  )
  set_target_properties(TestCommonWrite_f TestCommonRead_f PROPERTIES
    LINKER_LANGUAGE Fortran
  )
  if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestCommonWrite_f adios2_fortran_mpi MPI::MPI_Fortran)
    target_link_libraries(TestCommonRead_f adios2_fortran_mpi MPI::MPI_Fortran)
  else()
    target_link_libraries(TestCommonWrite_f adios2_fortran)
    target_link_libraries(TestCommonRead_f adios2_fortran)
  endif()
endif()

configure_file(
  run_test.py.gen.in
  ${CMAKE_CURRENT_BINARY_DIR}/run_test.py.gen
  @ONLY
)
file(GENERATE
  OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run_test.py.$<CONFIG>
  INPUT ${CMAKE_CURRENT_BINARY_DIR}/run_test.py.gen)

include (TestSupp.cmake )

if(ADIOS2_HAVE_MPI AND MPIEXEC_EXECUTABLE)
   # we want to know if mpiexec is a shell script or a binary executable
   file(READ ${MPIEXEC_EXECUTABLE} MPIFILEDATA LIMIT 1024 HEX)
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

set (SIMPLE_TESTS "1x1;NoReaderNoWait;TimeoutOnOpen;1x1.NoData;1x1.Modes;1x1.Attrs;1x1.Local;1x1.SharedNothing;1x1.SharedIO;1x1.SharedVar;1x1.SharedNothingSync;1x1.SharedIOSync;1x1.SharedVarSync;1x1EarlyExit")

set (SIMPLE_FORTRAN_TESTS "")
if(ADIOS2_HAVE_Fortran)
  set (SIMPLE_FORTRAN_TESTS "FtoC.1x1;CtoF.1x1;FtoF.1x1")
endif()

set (SPECIAL_TESTS "TimeoutReader.1x1;LatestReader.1x1;LatestReaderHold.1x1;DiscardWriter.1x1;1x1.NoPreload;1x1.ForcePreload;1x1LockGeometry")
if (MPIEXEC_IS_BINARY)
    # run_test.py can only kill readers/writers if mpiexec is not a shell script
    list(APPEND SPECIAL_TESTS "KillReadersSerialized.3x2;KillReaders3Max.3x6;KillWriter_2x2;KillWriterTimeout_2x2")
endif()

set (SIMPLE_MPI_TESTS "")
set (SIMPLE_MPI_FORTRAN_TESTS "")
if (ADIOS2_HAVE_MPI)
  set (SIMPLE_MPI_TESTS "2x1;1x2;3x5;5x3;DelayedReader_3x5;3x5LockGeometry;2x1.Local;1x2.Local;3x5.Local;5x3.Local;1x1.LocalVarying;5x3.LocalVarying;2x1ZeroDataVar;2x1ZeroDataR64;2x2.NoData;2x2.HalfNoData;2x1.SharedNothing;2x1.SharedIO;2x1.SharedVar;2x1.SharedNothingSync;2x1.SharedIOSync;2x1.SharedVarSync;3x5EarlyExit")
  list (APPEND SPECIAL_TESTS "2x1.NoPreload;2x3.ForcePreload;PreciousTimestep.3x2;PreciousTimestepDiscard.3x2")
  if (ADIOS2_HAVE_Fortran)
    set (SIMPLE_MPI_FORTRAN_TESTS "FtoC.3x5;CtoF.3x5;FtoF.3x5")
  endif()
endif()
 
set (SIMPLE_ZFP_TESTS "")
if (ADIOS2_HAVE_ZFP)
   set (SIMPLE_ZFP_TESTS "ZFPCompression.1x1;ZFPCompression.3x5")
endif()

set (ALL_SIMPLE_TESTS "")
list (APPEND ALL_SIMPLE_TESTS ${SIMPLE_TESTS} ${SIMPLE_FORTRAN_TESTS} ${SIMPLE_MPI_TESTS} ${SIMPLE_ZFP_TESTS})

#
#   Setup tests for SST engine
#
SET (BASIC_SST_TESTS "")
if(ADIOS2_HAVE_SST)
    list (APPEND BASIC_SST_TESTS ${ALL_SIMPLE_TESTS} ${SPECIAL_TESTS})
endif()


#  For the moment, only test the default comm pattern (Peer)
MutateTestSet( COMM_MIN_SST_TESTS "CommMin" writer "CPCommPattern=Min" "${BASIC_SST_TESTS}" )
MutateTestSet( COMM_PEER_SST_TESTS "CommPeer" writer "CPCommPattern=Peer" "${BASIC_SST_TESTS}" )

# temporarily remove PreciousTimestep CommPeer tests
list (REMOVE_ITEM COMM_PEER_SST_TESTS "PreciousTimestep")

MutateTestSet( FFS_SST_TESTS "FFS" writer "MarshalMethod=FFS" "${COMM_MIN_SST_TESTS};${COMM_PEER_SST_TESTS}" )
MutateTestSet( BP_SST_TESTS "BP" writer "MarshalMethod=BP" "${COMM_MIN_SST_TESTS};${COMM_PEER_SST_TESTS}" )

set (SST_TESTS "")
LIST (APPEND SST_TESTS ${FFS_SST_TESTS} ${BP_SST_TESTS})

# Zero Data tests are unreliable with SST and BP marshaling
list (FILTER SST_TESTS EXCLUDE REGEX "2x1ZeroData.*BP")

foreach(test ${SST_TESTS})
    add_common_test(${test} SST)
endforeach()

#
#   Setup tests for InSituMPI engine
#
if(ADIOS2_HAVE_MPI AND ADIOS2_RUN_MPMD_TESTS)
    set (SIMPLE_IMPI_TESTS "1x1;TimeoutOnOpen;1x1.Modes;1x1.Attrs;1x1.Local;1x1.SharedNothing;1x1.SharedIO;1x1.SharedVar;1x1.SharedNothingSync;1x1.SharedIOSync;1x1.SharedVarSync;2x1.SharedNothing;2x1.SharedIO;2x1.SharedVar;2x1.SharedNothingSync;2x1.SharedIOSync;2x1.SharedVarSync")
    set (INSITU_TESTS ${SIMPLE_IMPI_TESTS} ${SIMPLE_FORTRAN_TESTS} ${SIMPLE_MPI_TESTS} ${SIMPLE_ZFP_TESTS})
    # Tests that don't work for InSitu
    list (REMOVE_ITEM INSITU_TESTS "TimeoutOnOpen" "1x1.Modes" "1x1.Attrs")
    # Local Vars don't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX ".*Local$")
    # Local Vars don't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX ".*LocalVarying$")
    # Multiple streams don't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX "x1.Shared")
    # Fortran Destination doesn't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX "toF")
    # Zero size Data doesn't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX "ZeroData")
    # DelayedReader doesn't make sense for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX "DelayedReader.*")
    # Fortran scalar reads don't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX "(FtoF|CtoF).*")
    # No locking geometry
    list (FILTER INSITU_TESTS EXCLUDE REGEX "LockGeometry")
    # The nobody-writes-data-in-a-timestep tests don't work for InSitu
    list (FILTER INSITU_TESTS EXCLUDE REGEX ".*NoData$")
    # Readers can't exit early in INSITU
    list (FILTER INSITU_TESTS EXCLUDE REGEX ".*EarlyExit$")
    foreach(test ${INSITU_TESTS})
        add_common_test(${test} InSituMPI)
    endforeach()
endif()

if(ADIOS2_HAVE_SSC AND ADIOS2_RUN_MPMD_TESTS)
    set (SSC_BASE_TESTS "1x1;1x1.Attrs;2x1;1x2;2x1ZeroDataVar;2x1ZeroDataR64;3x5;5x3;3x5LockGeometry;TimeoutOnOpen")
    set (SSC_TESTS  ${SIMPLE_FORTRAN_TESTS} ${SIMPLE_MPI_FORTRAN_TESTS} ${SSC_BASE_TESTS})
    foreach(test ${SSC_TESTS})
        add_common_test(${test} SSC)
    endforeach()
endif()

#
#   Setup tests for BP engines
#

if(NOT MSVC)    # not on windows
    set (BP_TESTS ${ALL_SIMPLE_TESTS})
    # Delayed reader not worth testing on file engines
    list (FILTER BP_TESTS EXCLUDE REGEX "DelayedReader")
    # SharedVars fail with BP*
    list (FILTER BP_TESTS EXCLUDE REGEX ".*SharedVar$")
    # The nobody-writes-data-in-a-timestep tests don't work for any BP file engine
    list (FILTER BP_TESTS EXCLUDE REGEX ".*NoData$")
    foreach(test ${BP_TESTS})
        add_common_test(${test} BP4)
    endforeach()
    # BP3 fails to handle lots of timesteps
    list (FILTER BP_TESTS EXCLUDE REGEX "Bulk.*")
    foreach(test ${BP_TESTS})
        add_common_test(${test} BP3)
    endforeach()
endif()


#
#   Setup streaming tests for BP4 engine
#
if(NOT MSVC)    # not on windows
    # BP4 streaming tests start with all the simple tests, but with a timeout added on open
    LIST (APPEND BP4_STREAM_TESTS ${ALL_SIMPLE_TESTS} ${SPECIAL_TESTS})
    MutateTestSet( BP4_STREAM_TESTS "BPS" reader "OpenTimeoutSecs=10" "${BP4_STREAM_TESTS}")
    # SharedVars fail with BP4_streaming*
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*SharedVar.BPS$")
    # Discard not a feature of BP4
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*DiscardWriter.1x1.*BPS$")
    # PreciousTimestep not a feature of BP4
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*Precious.*BPS$")
    # Timeout on Reader BeginStep failing in BP4
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*TimeoutReader.1x1.*BPS$")
    # LatestTimestep not a feature of BP4
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*LatestReader.*BPS$")
    # Preload not a feature of BP4_streaming
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*Preload.*BPS$")
    # KillWriter fails with BP4
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*KillWriter.*BPS$")
    # SharedVars fail with BP4_streaming*
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*SharedVar.BPS$")
    # Local fail with BP4_streaming*
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*Local.BPS$")
    # Fortran scalar reads don't work for BP4 streaming
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX "(FtoF|CtoF).*")
    # Local Vars have a heisen failure for BP4 streaming
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*LocalVarying.BPS$")
    # The nobody-writes-data-in-a-timestep tests don't work for any BP file engine
    list (FILTER BP4_STREAM_TESTS EXCLUDE REGEX ".*NoData.BPS$")
    
    foreach(test ${BP4_STREAM_TESTS})
        add_common_test(${test} BP4_stream)
    endforeach()

    # add burst buffer tests
    MutateTestSet( BP4_BBSTREAM_TESTS "BB" writer "BurstBufferPath=bb,BurstBufferVerbose=2" "${BP4_STREAM_TESTS}")
    foreach(test ${BP4_BBSTREAM_TESTS})
        add_common_test(${test} BP4_stream)
    endforeach()
    
endif()

#
#   Setup tests for HDF5 engine
#
if(NOT MSVC AND ADIOS2_HAVE_HDF5 AND (NOT ADIOS2_HAVE_MPI OR HDF5_IS_PARALLEL))    # not on windows and only if we have HDF5
    set (HDF5_TESTS ${ALL_SIMPLE_TESTS})
    # Delayed reader not worth testing on file engines
    list (FILTER HDF5_TESTS EXCLUDE REGEX "DelayedReader")
    # Local variables don't work on hdf5 
    list (FILTER HDF5_TESTS EXCLUDE REGEX ".*Local")
    # Oddly, some multi-writer tests fail
    list (FILTER HDF5_TESTS EXCLUDE REGEX "[2345]x")
    # The nobody-writes-data-in-a-timestep tests don't work with HDF5
    list (FILTER HDF5_TESTS EXCLUDE REGEX ".*HalfNoData$")
    foreach(test ${HDF5_TESTS})
        add_common_test(${test} HDF5)
    endforeach()
endif()
