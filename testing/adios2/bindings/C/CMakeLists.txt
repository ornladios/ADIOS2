#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#


function(add_c_test testname mpi)
  add_executable(Test${testname}_c Test${testname}.cpp)
  target_link_libraries(Test${testname}_c adios2 gtest)
  if(mpi)
    target_link_libraries(Test${testname}_c MPI::MPI_C)
    gtest_add_tests(TARGET Test${testname}_c
      TEST_LIST gtest_added_tests
      TEST_PREFIX Bindings.C.
      EXEC_WRAPPER ${MPIEXEC_COMMAND}
    )
    set_tests_properties(${gtest_added_tests} PROPERTIES
      PROCESSORS ${NUM_TEST_PROCS}
    )
  else()
    gtest_add_tests(TARGET Test${testname}_c
      TEST_LIST gtest_added_tests
      TEST_PREFIX Bindings.C.
    )
  endif()
endfunction()

add_c_test(BPWriteTypes "${ADIOS2_HAVE_MPI}")
add_c_test(BPWriteReadMultiblock "${ADIOS2_HAVE_MPI}")
add_c_test(NullWriteRead "${ADIOS2_HAVE_MPI}")
if(ADIOS2_HAVE_MPI)
  add_c_test(BPWriteAggregateReadLocal TRUE)
endif()
