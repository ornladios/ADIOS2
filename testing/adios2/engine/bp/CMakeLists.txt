# ------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
# ------------------------------------------------------------------------------#
include(ADIOSFunctions)

set(BP3_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp3)
set(BP4_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp4)
set(BP5_DIR ${CMAKE_CURRENT_BINARY_DIR}/bp5)
set(DAOS_DIR ${CMAKE_CURRENT_BINARY_DIR}/daos)
set(XROOTD_DIR ${CMAKE_CURRENT_BINARY_DIR}/xrootd)
set(KVCACHE_DIR ${CMAKE_CURRENT_BINARY_DIR}/kvcache)
set(FS_DIR ${CMAKE_CURRENT_BINARY_DIR}/filestream)
file(MAKE_DIRECTORY ${BP3_DIR})
file(MAKE_DIRECTORY ${BP4_DIR})
file(MAKE_DIRECTORY ${BP5_DIR})
file(MAKE_DIRECTORY ${DAOS_DIR})
file(MAKE_DIRECTORY ${XROOTD_DIR})
file(MAKE_DIRECTORY ${KVCACHE_DIR})
file(MAKE_DIRECTORY ${FS_DIR})

set(BP5_ASYNC_DIR ${BP5_DIR}/async)
file(MAKE_DIRECTORY ${BP5_ASYNC_DIR}/tls-guided)
file(MAKE_DIRECTORY ${BP5_ASYNC_DIR}/tls-naive)
file(MAKE_DIRECTORY ${BP5_ASYNC_DIR}/ews-guided)
file(MAKE_DIRECTORY ${BP5_ASYNC_DIR}/ews-naive)

macro(bp5_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP5
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
  )
endmacro()

macro(bp3_bp4_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP3
    WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
endmacro()

macro(bp4_bp5_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP5
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
  )
endmacro()

macro(bp_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP3
    WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP4
    WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .BP5
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
  )
endmacro()

macro(async_gtest_add_tests_helper testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .Async.BP5.TLS.Guided
    WORKING_DIRECTORY ${BP5_ASYNC_DIR}/tls-guided EXTRA_ARGS "BP5" "AggregationType=TwoLevelShm,AsyncWrite=Guided"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .Async.BP5.TLS.Naive
    WORKING_DIRECTORY ${BP5_ASYNC_DIR}/tls-naive EXTRA_ARGS "BP5" "AggregationType=TwoLevelShm,AsyncWrite=Naive"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .Async.BP5.EWS.Guided
    WORKING_DIRECTORY ${BP5_ASYNC_DIR}/ews-guided EXTRA_ARGS "BP5" "AggregationType=EveryoneWritesSerial,AsyncWrite=Guided"
  )
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BP. .Async.BP5.EWS.Naive
    WORKING_DIRECTORY ${BP5_ASYNC_DIR}/ews-naive EXTRA_ARGS "BP5" "AggregationType=EveryoneWritesSerial,AsyncWrite=Naive"
  )
endmacro()

if(ADIOS2_HAVE_Fortran)
  macro(bp_gtest_add_tests_helper_Fortran testname mpi)
    # message(STATUS "Creating Fortran test ${testname} ${mpi}")
    gtest_add_tests_helper_Fortran(${testname} ${mpi} BP Engine.BP. .BP3
      ${BP3_DIR} "BP3"
    )
    gtest_add_tests_helper_Fortran(${testname} ${mpi} BP Engine.BP. .BP4
      ${BP4_DIR} "BP4"
    )
    gtest_add_tests_helper_Fortran(${testname} ${mpi} BP Engine.BP. .BP5
      ${BP5_DIR} "BP5"
    )
  endmacro()
else()
  macro(bp_gtest_add_tests_helper_Fortran testname mpi)
    message(STATUS "Skip creating Fortran test ${testname} ${mpi}")
  endmacro()
endif()

add_subdirectory(operations)

# These tests should be *very* fast
set(CTEST_TEST_TIMEOUT 10)

bp_gtest_add_tests_helper(WriteReadADIOS2 MPI_ALLOW)
async_gtest_add_tests_helper(WriteReadADIOS2 MPI_ALLOW)

gtest_add_tests_helper(WriteReadFlatten MPI_ONLY BP Engine.BP. .BP5 WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5" )

if(UNIX)
  gtest_add_tests_helper(LargeBlocks MPI_NONE BP Engine.BP. .BP5 WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5" )
  set_tests_properties(Engine.BP.LargeBlocks.MultiBlock.BP5.Serial PROPERTIES SKIP_REGULAR_EXPRESSION "SKIP:" RUN_SERIAL TRUE)
  target_link_libraries(Test.Engine.BP.LargeBlocks.Serial adios2sys)
endif()

bp_gtest_add_tests_helper(WriteReadADIOS2fstream MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadADIOS2stdio MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAsStreamADIOS2 MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAsStreamADIOS2_Threads MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAttributes MPI_ALLOW)
bp_gtest_add_tests_helper(FStreamWriteReadHighLevelAPI MPI_ALLOW)
bp_gtest_add_tests_helper(WriteFlushRead MPI_ALLOW)
bp_gtest_add_tests_helper(WriteMultiblockRead MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadMultiblock MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadVector MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadAttributesMultirank MPI_ALLOW)
bp_gtest_add_tests_helper(LargeMetadata MPI_ALLOW)
bp5_gtest_add_tests_helper(WriteStatsOnly MPI_ALLOW)

if (ADIOS2_HAVE_MPI)
  # Extra arguments: aggregation type, num subfiles, num timesteps, verbose level
  gtest_add_tests_helper(DataSizeAggregate MPI_ONLY BP Engine.BP. .BP5.DSB
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "DataSizeBased" "3" "5" "2"
  )
endif()

set(BP5LargeMeta "Engine.BP.BPLargeMetadata.BPWrite1D_LargeMetadata.BP5.Serial")

if ((NOT WIN32) AND ADIOS2_HAVE_SST)
   # prototype for remote server testing
   # (we don't really use SST here, just EVPath, but ADIOS2_HAVE_SST is the most relevant conditional)

   macro(add_get_remote_tests_helper testname)
      add_test(NAME "Remote.BP${testname}.GetRemote" COMMAND Test.Engine.BP.${testname}.Serial bp5 RemoteGet)
      set_tests_properties(Remote.BP${testname}.GetRemote PROPERTIES FIXTURES_REQUIRED Server ENVIRONMENT "DoRemote=1"  WORKING_DIRECTORY ${REMOTE_DIR})
   endmacro()

   macro(add_file_remote_tests_helper testname)
      add_test(NAME "Remote.BP${testname}.FileRemote" COMMAND Test.Engine.BP.${testname}.Serial bp5 RemoteFile)
      set_tests_properties(Remote.BP${testname}.FileRemote PROPERTIES FIXTURES_REQUIRED Server ENVIRONMENT "DoFileRemote=1" WORKING_DIRECTORY ${REMOTE_DIR})
   endmacro()

   add_test(NAME remoteServerSetup   COMMAND adios2_remote_server -background -v -l /tmp/server_output)
   set_tests_properties(remoteServerSetup         PROPERTIES FIXTURES_SETUP    Server WORKING_DIRECTORY ${REMOTE_DIR})

   add_test(NAME remoteServerCleanup COMMAND adios2_remote_server -kill_server -d /tmp/server_output)
   set_tests_properties(remoteServerCleanup         PROPERTIES FIXTURES_CLEANUP    Server WORKING_DIRECTORY ${REMOTE_DIR})

  if (ADIOS2_HAVE_KVCACHE)
    add_test(NAME redisServerSetup COMMAND ${REDIS_SERVER_BINARY} --daemonize yes)
    set_tests_properties(redisServerSetup PROPERTIES FIXTURES_SETUP RedisServer WORKING_DIRECTORY ${KVCACHE_DIR})

    add_test(NAME redisServerCleanup COMMAND ${REDIS_CLI_BINARY} shutdown)
    set_tests_properties(redisServerCleanup PROPERTIES FIXTURES_CLEANUP RedisServer WORKING_DIRECTORY ${KVCACHE_DIR})

    macro(add_get_kvcache_tests_helper testname)
      add_test(NAME "Remote.BP${testname}.GetKVCache" COMMAND Test.Engine.BP.${testname}.Serial bp5)
      set_tests_properties(Remote.BP${testname}.GetKVCache PROPERTIES FIXTURES_REQUIRED "Server;RedisServer" ENVIRONMENT "DoRemote=1;useKVCache=1" WORKING_DIRECTORY ${KVCACHE_DIR})

    endmacro()

    add_get_kvcache_tests_helper(WriteReadADIOS2stdio)
    add_get_kvcache_tests_helper(WriteMemorySelectionRead)
  endif()

   ##### add remote tests below this line
   add_get_remote_tests_helper(WriteReadADIOS2stdio)
   add_get_remote_tests_helper(WriteMemorySelectionRead)
   add_file_remote_tests_helper(WriteMemorySelectionRead)
endif()

if (ADIOS2_HAVE_XRootD)
   macro(add_get_xrremote_tests_helper testname)
      add_test(NAME "Remote.BP${testname}.GetXRRemote" COMMAND Test.Engine.BP.${testname}.Serial bp5)
      set_tests_properties(Remote.BP${testname}.GetXRRemote PROPERTIES FIXTURES_REQUIRED XRServer ENVIRONMENT "DoXRootD=1" WORKING_DIRECTORY ${XROOTD_DIR})
   endmacro()

   add_test(NAME generateXRootDConfig COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generateXRootDConfig.sh $<TARGET_FILE:adios2_xrootd>  )
   add_test(NAME remoteXRServerSetup COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/start_xrootd.sh ${XROOTD_SERVER_BINARY} ${CMAKE_CURRENT_BINARY_DIR})

   set_tests_properties(remoteXRServerSetup PROPERTIES FIXTURES_SETUP XRServer WORKING_DIRECTORY ${XROOTD_DIR})
   set_tests_properties(remoteXRServerSetup PROPERTIES DEPENDS "generateXRootDConfig")

   add_test(NAME remoteXRServerCleanup COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/kill_xrootd.sh )
   set_tests_properties(remoteXRServerCleanup         PROPERTIES FIXTURES_CLEANUP    XRServer)

   ##### add remote tests below this line
   add_get_xrremote_tests_helper(WriteReadADIOS2stdio)
   add_get_xrremote_tests_helper(WriteMemorySelectionRead)
endif()

if(ADIOS2_HAVE_MPI)
  list(APPEND BP5LargeMeta "Engine.BP.BPLargeMetadata.BPWrite1D_LargeMetadata.BP5.MPI" "Engine.BP.BPLargeMetadata.ManyLongStrings.BP5.MPI")
endif()

set_tests_properties(${BP5LargeMeta} PROPERTIES RUN_SERIAL TRUE)

bp_gtest_add_tests_helper(MultiSpan MPI_NONE)
bp_gtest_add_tests_helper(WriteMemorySelectionRead MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadLocalVariables MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadLocalVariablesSel MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadLocalVariablesSelHighLevel MPI_ALLOW)
bp_gtest_add_tests_helper(ChangingShape MPI_ALLOW)
bp_gtest_add_tests_helper(ChangingShapeWithinStep MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadBlockInfo MPI_ALLOW)
bp_gtest_add_tests_helper(WriteReadVariableSpan MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(TimeAggregation MPI_ALLOW)
bp_gtest_add_tests_helper(NoXMLRecovery MPI_ALLOW)
bp3_bp4_gtest_add_tests_helper(StepsFileGlobalArray MPI_ALLOW)
bp_gtest_add_tests_helper(StepsFileLocalArray MPI_ALLOW)

bp_gtest_add_tests_helper(SelectionsOnRowMajorData MPI_NONE)
bp_gtest_add_tests_helper(SelectionsOnColumnMajorData MPI_NONE)

# Fortran writer C++ reader test
if(ADIOS2_HAVE_Fortran)
  bp_gtest_add_tests_helper_Fortran(FortranToCppWriter MPI_ONLY)
  bp_gtest_add_tests_helper(FortranToCppReader MPI_ONLY)

  # SetupTestPipeline(Engine.BP.BPFortranToCPPWriter.BP5.MPI ";Engine.BP.BPFortranToCPPReader.ADIOS2BPFortranToCppRead.BP5.MPI" FALSE)
endif(ADIOS2_HAVE_Fortran)

if(NOT MSVC)
  bp_gtest_add_tests_helper(BufferSize MPI_NONE)
endif()

if(ADIOS2_HAVE_MPI)
  bp_gtest_add_tests_helper(WriteAggregateRead MPI_ONLY)
endif()

macro(bp5agg_gtest_add_tests testname mpi)
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BPAGG. .BP5.EWS
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5" "EveryoneWritesSerial")
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BPAGG. .BP5.EW
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5" "EveryoneWrites")
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BPAGG. .BP5.TLS
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5" "TwoLevelShm")
  gtest_add_tests_helper(${testname} ${mpi} BP Engine.BPAGG. .BP5.DSB
    WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5" "DataSizeBased")
endmacro()

# BP5 only for now, so test all the aggregators while we're at it
bp5agg_gtest_add_tests(ParameterSelectSteps MPI_ALLOW)
bp5agg_gtest_add_tests(AppendAfterSteps MPI_ALLOW)
bp5agg_gtest_add_tests(DirectIO MPI_NONE)
bp5agg_gtest_add_tests(ReadMultithreaded MPI_NONE)
bp5agg_gtest_add_tests(NewFileAppendMode MPI_NONE)
bp5agg_gtest_add_tests(StepsFileGlobalArray MPI_ALLOW)

# Only a single test is enough, pick the latest engine
gtest_add_tests_helper(AccuracyDefaults MPI_NONE BP Engine.BP. .BP5
  WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
)

# BP3 only for now
gtest_add_tests_helper(WriteNull MPI_ALLOW BP Engine.BP. .BP3
  WORKING_DIRECTORY ${BP3_DIR} EXTRA_ARGS "BP3"
)

# BP4 and BP5 but NOT BP3
bp4_bp5_gtest_add_tests_helper(WriteAppendReadADIOS2 MPI_ALLOW)
bp4_bp5_gtest_add_tests_helper(JoinedArray MPI_ALLOW)
bp4_bp5_gtest_add_tests_helper(OpenWithMetadata MPI_NONE)

# BP4 only for now
# gtest_add_tests_helper(WriteAppendReadADIOS2 MPI_ALLOW BP Engine.BP. .BP4
# WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
# )
gtest_add_tests_helper(StepsInSituGlobalArray MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
gtest_add_tests_helper(StepsInSituLocalArray MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)
#gtest_add_tests_helper(JoinedArray MPI_ALLOW BP Engine.BP. .BP4
#  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
#)

# InquireVaribleException only for BP4 because BP5 works differently
gtest_add_tests_helper(InquireVariableException MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)

gtest_add_tests_helper(InquireDefine MPI_ALLOW BP Engine.BP. .BP5
  WORKING_DIRECTORY ${BP5_DIR} EXTRA_ARGS "BP5"
)

gtest_add_tests_helper(InquireDefine MPI_ALLOW BP Engine.BP. .BP4
  WORKING_DIRECTORY ${BP4_DIR} EXTRA_ARGS "BP4"
)

# FileStream is BP4 + StreamReader=true
gtest_add_tests_helper(StepsInSituGlobalArray MPI_ALLOW BP Engine.BP. .FileStream
  WORKING_DIRECTORY ${FS_DIR} EXTRA_ARGS "FileStream"
)
gtest_add_tests_helper(StepsInSituLocalArray MPI_ALLOW BP Engine.BP. .FileStream
  WORKING_DIRECTORY ${FS_DIR} EXTRA_ARGS "FileStream"
)
