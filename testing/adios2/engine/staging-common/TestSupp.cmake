#
#
# This Cmake testing specification is unfortunately complex, and will
# likely be hard to maintain, but at the moment I don't know a better
# way to do it (contributions welcome).  The idea is that we have a set of tests that we'd
# like to run on staging engines.  Some tests may be applicable to
# some engines and not to others.  Some tests we want to run multiple
# times, but with different engine parameters set to test different
# modes of operation for the engines.
#
# The approach here is as follows:
#   - A test is specified by setting a CMake variable of the form
#     <testname>_CMD.  
#   - If the test requires a different time out than is usually
#     specified (30 seconds), then you can also define a variable of
#     the form <testname>_TIMEOUT to control the timeout.
#   - If the test requires special properties you can also define a variable of 
#     the form <testname>_PROPERTIES.
# 
#     BASIC TEST ADDITION
# Given the existence of the above variables, the cmake function
# "add_common_test(testname engine)" adds a CTest which runs the given
# test with a particular ADIOS2 engine, adding the engine's name to
# the test name to differentiate it from other tests.  For example,
# given the variable 1x1_CMD, set to
#  "run_staging_test -nw 1 -nr 1 -v -p TestCommon"
# and 1x1_PROPERTIES set to "RUN_SERIAL;1" then
# add_common_test(1x1 SST) ends up doing:
# 
#  add_test(NAME "Staging.1x1.SST"
#          COMMAND "run_staging_test -e SST -f Staging.1x1.SST -nw 1 -nr 1")
#  set_tests_properties(${testname} PROPERTIES TIMEOUT 30 RUN_SERIAL 1)
#
#    RUNNING TESTS WITH DIFFERENT ENGINE PARAMETERS
# 
# One of the design goals here is to enable running the same tests
# with different engine parameters, *_CMD strings can also contain the
# string "ENGINE_PARAMS".  This string is treated specially by the
# function MutateTestSet().  This function is takes a list of tests

# (I.E. things with _CMD strings defined like above) and creates a new
# set of tests where a specified engine parameter gets added to the in
# the location of the ENGINE_PARAMS string.  MutateTestSet takes 4 parameters:
# output_test_list, param_name, param_spec, and input test list.  For example
# MutateTestSet( COMM_MIN_SST_TESTS "CommMin" "CPCommPattern:Min" "${BASIC_SST_TESTS}" )
# If BASIC_SST_TESTS contains "1x1" as defined above, MutateTestSet
# will add the test "1x1.CommMin", by defining the variable
# 1x1.CommMin_CMD, using the original value of 1x1_CMD buth with
# "CPCommPattern:Min: added to the ENGINE_PARAMS location (if
# present).  Any 1x1_TIMEOUT and 1x1_PROPERTIES values will also be
# propogated to 1x1.CommMin_TIMEOUT and 1x1.CommMin_PROPERTIES.
# "1x1.CommMin" will also be added to the output test list.
# 
# Note that MutateTestSet() can be used multiple times to add multiple
# engine params.  The ENGINE_PARAMS string is retained in the
# resulting _CMD strings until add_common_test() which removes it.
#
# Change the STAGING_COMMON_TEST_SUPP_VERBOSE value to ON for debugging output
#
set (STAGING_COMMON_TEST_SUPP_VERBOSE OFF)

set (1x1_CMD "run_test.py -nw 1 -nr 1 --warg=ENGINE_PARAMS")
set (2x1_CMD "run_test.py -nw 2 -nr 1 --warg=ENGINE_PARAMS")
set (1x2_CMD "run_test.py -nw 1 -nr 2 --warg=ENGINE_PARAMS")
set (3x5_CMD "run_test.py -nw 3 -nr 5 --warg=ENGINE_PARAMS")
set (5x3_CMD "run_test.py -nw 5 -nr 3 --warg=ENGINE_PARAMS")
set (1x1.Local_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteLocal -r TestCommonReadLocal --warg=ENGINE_PARAMS")
set (2x1.Local_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteLocal -r TestCommonReadLocal --warg=ENGINE_PARAMS")
set (1x2.Local_CMD "run_test.py -nw 1 -nr 2  -w TestCommonWriteLocal -r TestCommonReadLocal --warg=ENGINE_PARAMS")
set (3x5.Local_CMD "run_test.py -nw 3 -nr 5  -w TestCommonWriteLocal -r TestCommonReadLocal --warg=ENGINE_PARAMS")
set (5x3.Local_CMD "run_test.py -nw 5 -nr 3  -w TestCommonWriteLocal -r TestCommonReadLocal --warg=ENGINE_PARAMS")
set (DelayedReader_3x5_CMD "run_test.py -rd 5 -nw 3 -nr 5 --warg=ENGINE_PARAMS")
set (FtoC.3x5_CMD "run_test.py -nw 3 -nr 5  -w TestCommonWrite_f -r TestCommonRead --warg=ENGINE_PARAMS")
set (FtoF.3x5_CMD "run_test.py -nw 3 -nr 5  -w TestCommonWrite_f -r TestCommonRead_f --warg=ENGINE_PARAMS")
set (1x1.SharedNothing_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--write_mode --warg=deferred")
set (1x1.SharedIO_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_io --rarg=--shared_io --warg=--write_mode --warg=deferred")
set (1x1.SharedVar_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_var --rarg=--shared_var --warg=--write_mode --warg=deferred")
set (1x1.SharedNothingSync_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--write_mode --warg=sync")
set (1x1.SharedIOSync_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_io --rarg=--shared_io --warg=--write_mode --warg=sync")
set (1x1.SharedVarSync_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_var --rarg=--shared_var --warg=--write_mode --warg=sync")

set (2x1.SharedNothing_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--write_mode --warg=deferred")
set (2x1.SharedIO_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_io --rarg=--shared_io --warg=--write_mode --warg=deferred")
set (2x1.SharedVar_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_var --rarg=--shared_var --warg=--write_mode --warg=deferred")
set (2x1.SharedNothingSync_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--write_mode --warg=sync")
set (2x1.SharedIOSync_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_io --rarg=--shared_io --warg=--write_mode --warg=sync")
set (2x1.SharedVarSync_CMD "run_test.py -nw 2 -nr 1  -w TestCommonWriteShared -r TestCommonReadShared --warg=ENGINE_PARAMS --warg=--shared_var --rarg=--shared_var --warg=--write_mode --warg=sync")


# NoReaderNoWait runs a writer with the RendezvousReaderCount = 0 and then never spawns a reader.  The test should run to termination and execute cleanly
set (NoReaderNoWait_CMD "run_test.py -nw 1 -nr 0 --warg=RendezvousReaderCount:0,QueueLimit:3,QueueFullPolicy:discard,ENGINE_PARAMS")

# TimeoutOnOpen runs a reader but never spawns a writ.  The Open should run to timeout and throw an exception
set (TimeoutOnOpen_CMD "run_test.py -nw 0 -nr 1 --rarg=--expect_timeout --rarg=OpenTimeoutSecs:10,ENGINE_PARAMS")

# The Modes test checks to see that we can mix and match Put Sync and Deferred modes and still get good data
set (Modes_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteModes -r TestCommonRead --warg=ENGINE_PARAMS")

# 1x1.Attrs tests writing and reading of attributes defined before Open
set (1x1.Attrs_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWriteAttrs -r TestCommonReadAttrs --warg=ENGINE_PARAMS")

# Basic Fortran tests, Fortran to C, C to Fortran and Fortran to Fortran
set (FtoC.1x1_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWrite_f -r TestCommonRead --warg=ENGINE_PARAMS")
set (CtoF.1x1_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWrite -r TestCommonRead_f --warg=ENGINE_PARAMS")
set (FtoF.1x1_CMD "run_test.py -nw 1 -nr 1  -w TestCommonWrite_f -r TestCommonRead_f --warg=ENGINE_PARAMS")

# Tests for ZFP compression (where supported by an engine param)
set (ZFPCompression.1x1_CMD "run_test.py -nw 1 -nr 1 --warg=CompressionMethod:zfp,ENGINE_PARAMS" )
set (ZFPCompression.3x5_CMD "run_test.py -nw 3 -nr 5 --warg=CompressionMethod:zfp,ENGINE_PARAMS" )

# Test if writer will survive readers departing unexpectedly
set (KillReadersSerialized_CMD "run_test.py --test_protocol kill_readers  -nw 3 -nr 2 --max_readers 1 --warg=RendezvousReaderCount:0,ENGINE_PARAMS --rarg=--ignore_time_gap")
set (KillReadersSerialized_TIMEOUT "300")
set (KillReadersSerialized_PROPERTIES "RUN_SERIAL;1")
set (KillReaders3Max_CMD "run_test.py --test_protocol kill_readers  -nw 3 -nr 2 --max_readers 3 --warg=RendezvousReaderCount:0,ENGINE_PARAMS --rarg=--ignore_time_gap")
set (KillReaders3Max_TIMEOUT "300")
set (KillReaders3Max_PROPERTIES "RUN_SERIAL;1")

set (KillWriter_2x2_CMD "run_test.py --test_protocol kill_writer   -nw 2 -nr 2 --interval 2 --warg=RendezvousReaderCount:1,ENGINE_PARAMS --rarg=--expect_writer_failure --rarg=--num_steps --rarg=1000")
set (KillWriterTimeout_2x2_CMD "run_test.py --test_protocol kill_writer -nw 2 -nr 2 --interval 2 --warg=RendezvousReaderCount:1,ENGINE_PARAMS --rarg=--expect_writer_failure --rarg=--num_steps --rarg=1000 --rarg=--non_blocking")

set (PreciousTimestep_CMD "run_test.py --test_protocol kill_readers  -nw 3 -nr 2 --max_readers 2 --warg=FirstTimestepPrecious:true,RendezvousReaderCount:0,ENGINE_PARAMS --rarg=--ignore_time_gap --rarg=--precious_first")

set (PreciousTimestep_TIMEOUT "300")
set (PreciousTimestep_PROPERTIES "RUN_SERIAL;1")

set (PreciousTimestepDiscard_CMD "run_test.py --test_protocol kill_readers  -nw 3 -nr 2 --max_readers 2 --warg=FirstTimestepPrecious:true,RendezvousReaderCount:0,QueueLimit:3,QueueFullPolicy:discard,ENGINE_PARAMS --rarg=--ignore_time_gap --rarg=--precious_first --rarg=--discard --warg=--ms_delay --warg=500")
set (PreciousTimestepDiscard_TIMEOUT "300")
set (PreciousTimestepDiscard_PROPERTIES "RUN_SERIAL;1")

# Readers using BeginStep with timeout.  Here we run the writer with a longer delay to make the reader timeout
set (TimeoutReader_CMD "run_test.py --test_protocol one_client -nw 1 -nr 1 --rarg=--non_blocking --warg=--ms_delay --warg=2000 --warg=--engine_params --warg=ENGINE_PARAMS")
set (TimeoutReader_TIMEOUT "60")
set (TimeoutReader_PROPERTIES "RUN_SERIAL;1")

# Readers using LatestAvailable   Writer runs faster than reader, so we expect misses
set (LatestReader_CMD "run_test.py --test_protocol one_client -nw 1 -nr 1 --warg=--ms_delay --warg=250 --warg=--engine_params --warg=ENGINE_PARAMS --rarg=--latest --rarg=--long_first_delay")
set (LatestReader_PROPERTIES "RUN_SERIAL;1")

set (LatestReaderHold_CMD "run_test.py --test_protocol one_client -nw 1 -nr 1 --warg=--ms_delay --warg=250 --warg=--engine_params --warg=ENGINE_PARAMS --rarg=--latest --rarg=--long_first_delay --rarg=--delay_while_holding")
set (LatestReaderHold_PROPERTIES "RUN_SERIAL;1")

# A faster writer and a queue policy that will cause timesteps to be discarded
set (DiscardWriter_CMD "run_test.py --test_protocol one_client -nw 1 -nr 1 --warg=--engine_params --warg=QueueLimit:1,QueueFullPolicy:discard,ENGINE_PARAMS --warg=--ms_delay --warg=250 --rarg=--discard")

function(remove_engine_params_placeholder dst_str src_str )
    string(REGEX REPLACE "([^ 		  ]*),ENGINE_PARAMS" "\\1" src_str "${src_str}")
    if ("${src_str}" MATCHES "ENGINE_PARAMS")
       # empty engine params remains
       string(REGEX REPLACE "--warg=--engine_params --warg=ENGINE_PARAMS" "" src_str "${src_str}")       
       string(REGEX REPLACE "--rarg=--engine_params --rarg=ENGINE_PARAMS" "" src_str "${src_str}")       
       string(REGEX REPLACE "--warg=ENGINE_PARAMS" "" src_str "${src_str}")       
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
  if (STAGING_COMMON_TEST_SUPP_VERBOSE)
    message (STATUS "Setting ${out_list} to ${tmp_list}")
  endif()
  set(${out_list} ${tmp_list} PARENT_SCOPE)
endfunction()

function(add_common_test basename engine)
    set(testname "Staging.${basename}.${engine}")
    if ("${${basename}_CMD}" STREQUAL "") 
       message(SEND_ERROR "Staging-Common test ${basename} has no defined ${basename}_CMD")
    endif()
    string (CONCAT command "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/" ${${basename}_CMD})
    remove_engine_params_placeholder(command  "${command}")
    separate_arguments(command)
    list(INSERT command 1 "${engine}" "${testname}")
    add_test(
    	NAME ${testname}
	COMMAND ${command})
    set_tests_properties(${testname} PROPERTIES RUN_SERIAL 1)
    if (STAGING_COMMON_TEST_SUPP_VERBOSE)
	message ( STATUS "Adding test \"${testname}\" COMMAND \"${command}\"")
    endif()
    set (timeout "${${basename}_TIMEOUT}")
    if ("${timeout}" STREQUAL "")
       set (timeout "30")
    endif()

    set_tests_properties(${testname} PROPERTIES TIMEOUT ${timeout} ${${basename}_PROPERTIES} )
endfunction()

function(from_hex HEX DEC)
    string(TOUPPER "${HEX}" HEX)
    set(_res 0)
    string(LENGTH "${HEX}" _strlen)

    while(_strlen GREATER 0)
        math(EXPR _res "${_res} * 16")
        string(SUBSTRING "${HEX}" 0 1 NIBBLE)
        string(SUBSTRING "${HEX}" 1 -1 HEX)
        if(NIBBLE STREQUAL "A")
            math(EXPR _res "${_res} + 10")
        elseif(NIBBLE STREQUAL "B")
            math(EXPR _res "${_res} + 11")
        elseif(NIBBLE STREQUAL "C")
            math(EXPR _res "${_res} + 12")
        elseif(NIBBLE STREQUAL "D")
            math(EXPR _res "${_res} + 13")
        elseif(NIBBLE STREQUAL "E")
            math(EXPR _res "${_res} + 14")
        elseif(NIBBLE STREQUAL "F")
            math(EXPR _res "${_res} + 15")
        else()
            math(EXPR _res "${_res} + ${NIBBLE}")
        endif()

        string(LENGTH "${HEX}" _strlen)
    endwhile()
    
    set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

