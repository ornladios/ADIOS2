set (1x1_CMD "run_staging_test -nw 1 -nr 1 -v -p TestCommon -arg ENGINE_PARAMS")
set (2x1_CMD "run_staging_test -nw 2 -nr 1 -v -p TestCommon -arg ENGINE_PARAMS")
set (1x2_CMD "run_staging_test -nw 1 -nr 2 -v -p TestCommon -arg ENGINE_PARAMS")
set (3x5_CMD "run_staging_test -nw 3 -nr 5 -v -p TestCommon -arg ENGINE_PARAMS")
set (5x3_CMD "run_staging_test -nw 5 -nr 3 -v -p TestCommon -arg ENGINE_PARAMS")
set (DelayedReader_3x5_CMD "run_staging_test -rd 5 -nw 3 -nr 5 -p TestCommon -arg ENGINE_PARAMS")
set (DelayedReaderBlocking_3x5_CMD "run_staging_test -rd 5 -nw 3 -nr 5 -v -p TestCommon -arg RendezvousReaderCount:0,QueueLimit:3,ENGINE_PARAMS -arg --expect_time_gap")
set (FtoC.3x5_CMD "run_staging_test -nw 3 -nr 5 -v -w TestCommonWrite_f -r TestCommonRead -arg ENGINE_PARAMS")
set (FtoF.3x5_CMD "run_staging_test -nw 3 -nr 5 -v -w TestCommonWrite_f -r TestCommonRead_f -arg ENGINE_PARAMS")

# NoReaderNoWait runs a writer with the RendezvousReaderCount = 0 and then never spawns a reader.  The test should run to termination and execute cleanly
set (NoReaderNoWait_CMD "run_staging_test -nw 1 -nr 0 -v -p TestCommon -arg RendezvousReaderCount:0,QueueLimit:3,QueueFullPolicy:discard,ENGINE_PARAMS")

# The Modes test checks to see that we can mix and match Put Sync and Deferred modes and still get good data
set (Modes_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWriteModes -r TestCommonRead -arg ENGINE_PARAMS")

# 1x1.Attrs tests writing and reading of attributes defined before Open
set (1x1.Attrs_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWriteAttrs -r TestCommonReadAttrs -arg ENGINE_PARAMS")

# Basic Fortran tests, Fortran to C, C to Fortran and Fortran to Fortran
set (FtoC.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite_f -r TestCommonRead -arg ENGINE_PARAMS")
set (CtoF.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite -r TestCommonRead_f -arg ENGINE_PARAMS")
set (FtoF.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -w TestCommonWrite_f -r TestCommonRead_f -arg ENGINE_PARAMS")

# Tests for ZFP compression (where supported by an engine param)
set (ZFPCompression.1x1_CMD "run_staging_test -nw 1 -nr 1 -v -p TestCommon -arg CompressionMethod:zfp,ENGINE_PARAMS" )
set (ZFPCompression.3x5_CMD "run_staging_test -nw 3 -nr 5 -v -p TestCommon -arg CompressionMethod:zfp,ENGINE_PARAMS" )

# Test if writer will survive readers departing unexpectedly
set (KillReadersSerialized_CMD "run_multi_test -test_protocol kill_readers  -verbose -nw 3 -nr 2 -max_readers 1 -warg RendezvousReaderCount:0,ControlTransport:sockets,ENGINE_PARAMS -rarg --ignore_time_gap")
set (KillReadersSerialized_TIMEOUT "300")
set (KillReadersSerialized_PROPERTIES "RUN_SERIAL;1")
set (KillReaders3Max_CMD "run_multi_test -test_protocol kill_readers  -verbose -nw 3 -nr 2 -max_readers 3 -warg RendezvousReaderCount:0,ControlTransport:sockets,ENGINE_PARAMS -rarg --ignore_time_gap")
set (KillReaders3Max_TIMEOUT "300")
set (KillReaders3Max_PROPERTIES "RUN_SERIAL;1")

# Readers using BeginStep with timeout.  Here we run the writer with a longer delay to make the reader timeout
set (TimeoutReader_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -rarg --non_blocking -warg --ms_delay -warg 2000 -warg --engine_params -warg ENGINE_PARAMS")
set (TimeoutReader_TIMEOUT "60")
set (TimeoutReader_PROPERTIES "RUN_SERIAL;1")

# Readers using LatestAvailable   Writer runs faster than reader, so we expect misses
set (LatestReader_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -warg --ms_delay -warg 250 -warg --engine_params -warg ENGINE_PARAMS -rarg --latest -rarg --long_first_delay")
set (LatestReader_PROPERTIES "RUN_SERIAL;1")

# A faster writer and a queue policy that will cause timesteps to be discarded
set (DiscardWriter_CMD "run_multi_test -test_protocol one_to_one -verbose -nw 1 -nr 1 -warg --engine_params -warg QueueLimit:1,QueueFullPolicy:discard,ENGINE_PARAMS -warg --ms_delay -warg 500 -rarg --discard")


function(remove_engine_params_placeholder dst_str src_str )
    string(REGEX REPLACE "([^ 		  ]*),ENGINE_PARAMS" "\\1" src_str "${src_str}")
    if ("{src_string}" MATCHES "ENGINE_PARAMS")
       # empty engine params remains
       string(REGEX REPLACE "-warg *--engine_params *-warg *ENGINE_PARAMS" "" src_str "${src_str}")       
       string(REGEX REPLACE "-arg *ENGINE_PARAMS" "" src_str "${src_str}")
    endif()

  set(${dst_str} ${src_str} PARENT_SCOPE)
endfunction()

function(add_engine_param dst_string src_string engine_param)
  set (tmp_string "")
  if("${src_string}" MATCHES ",ENGINE_PARAMS")
      string(REGEX REPLACE ",ENGINE_PARAMS" ",${engine_param},ENGINE_PARAMS" tmp_string ${src_string})
  elseif("${src_string}" MATCHES "ENGINE_PARAMS")
      string(REGEX REPLACE "ENGINE_PARAMS" "${engine_param},ENGINE_PARAMS" tmp_string ${src_string})
  else ()
       message(WARNING "ENGINE_PARAMS not found in string ${src_string}")
  endif ()

  set(${dst_string} "${tmp_string}" PARENT_SCOPE)
endfunction()

function(MutateTestSet out_list name param in_list )
  set (tmp_list "")
  FOREACH (basename ${in_list})
    set (modname "${basename}.${name}")
    if ("${${basename}_CMD}" STREQUAL "") 
       message(SEND_ERROR "Staging-Common MutateTestSet ${basename} has no defined ${basename}_CMD")
    endif()
    add_engine_param(tmp_cmd "${${basename}_CMD}" ${param})
    set(${modname}_CMD ${tmp_cmd} PARENT_SCOPE)
    if (NOT "${${basename}_TIMEOUT}" STREQUAL "") 
        set(${modname}_TIMEOUT ${${basename}_TIMEOUT} PARENT_SCOPE)
    endif()
    if (NOT "${${basename}_PROPERTIES}" STREQUAL "") 
        set(${modname}_PROPERTIES ${${basename}_PROPERTIES} PARENT_SCOPE)
    endif()
    LIST(APPEND tmp_list ${modname})
  endforeach()
  message (STATUS "Setting ${out_list} to ${tmp_list}")
  set(${out_list} ${tmp_list} PARENT_SCOPE)
endfunction()

function(add_common_test basename engine)
    set(testname "Staging.${basename}.${engine}")
    set(filename "Staging.${basename}.${engine}")
    if ("${${basename}_CMD}" STREQUAL "") 
       message(SEND_ERROR "Staging-Common test ${basename} has no defined ${basename}_CMD")
    endif()
    string (CONCAT command "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/" ${${basename}_CMD})
    remove_engine_params_placeholder(command  "${command}")
    separate_arguments(command)
    list(INSERT command 1 "-e" "${engine}" "-f" "${filename}")
    message (STATUS "Command is now ${command}")
    add_test(
    	NAME ${testname}
	COMMAND ${command})
    set (timeout "${${basename}_TIMEOUT}")
    if ("${timeout}" STREQUAL "")
       set (timeout "30")
    endif()

    message (STATUS "Timeout for test ${testname} is ${timeout}")
    set_tests_properties(${testname} PROPERTIES TIMEOUT ${timeout} ${${basename}_PROPERTIES} )
endfunction()

