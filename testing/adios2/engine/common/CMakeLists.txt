#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(Threads REQUIRED)

set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})

add_executable(TestEngineCommon  TestEngineCommon.cpp)
target_link_libraries(TestEngineCommon adios2 gtest ${CMAKE_THREAD_LIBS_INIT})
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestEngineCommon adios2 gtest_interface MPI::MPI_C)
endif()


# Note: extra arguments
#   1st arg: Engine
#   2nd arg: 1 for serialized execution, 0 for concurrent execution of Writer/Reader
#   3rd arg: engine parameters

gtest_add_tests(TARGET TestEngineCommon ${extra_test_args} 
                EXTRA_ARGS "BPFile" "1"
                TEST_SUFFIX _BPFile)
                
if(ADIOS2_HAVE_HDF5)
gtest_add_tests(TARGET TestEngineCommon ${extra_test_args} 
                EXTRA_ARGS "HDF5" "1"
                TEST_SUFFIX _HDF5)
endif()

if(ADIOS2_HAVE_SST)
gtest_add_tests(TARGET TestEngineCommon ${extra_test_args} 
                EXTRA_ARGS "SST" "0" "MarshalMethod:FFS"
                TEST_SUFFIX _SST_FFS) 
gtest_add_tests(TARGET TestEngineCommon ${extra_test_args} 
                EXTRA_ARGS "SST" "0" "MarshalMethod:BP"
                TEST_SUFFIX _SST_BP)
endif()

if(ADIOS2_HAVE_MPI)
gtest_add_tests(TARGET TestEngineCommon ${extra_test_args} 
                EXTRA_ARGS "InSituMPI" "0"
                TEST_SUFFIX _InSituMPI)
endif()

#if(ADIOS2_HAVE_DataMan)
#gtest_add_tests(TARGET TestEngineCommon ${extra_test_args} 
#                EXTRA_ARGS "DataMan" "0"
#                TEST_SUFFIX _DataMan)
#endif()


