#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(Threads REQUIRED)

set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})

add_executable(TestStagingMPMD  TestStagingMPMD.cpp)
target_link_libraries(TestStagingMPMD adios2 gtest ${CMAKE_THREAD_LIBS_INIT})
if(ADIOS2_HAVE_MPI)
    target_link_libraries(TestStagingMPMD adios2 gtest_interface MPI::MPI_C)
endif()

if(ADIOS2_HAVE_SST)
gtest_add_tests(TARGET TestStagingMPMD ${extra_test_args} 
                EXTRA_ARGS "SST"
                TEST_SUFFIX _SST)
gtest_add_tests(TARGET TestStagingMPMD ${extra_test_args} 
                EXTRA_ARGS "SST" "MarshalMethod:BP"
                TEST_SUFFIX _SST_BP)
endif()

if(ADIOS2_HAVE_MPI)
gtest_add_tests(TARGET TestStagingMPMD ${extra_test_args} 
                EXTRA_ARGS "InSituMPI"
                TEST_SUFFIX _InSituMPI)
endif()


