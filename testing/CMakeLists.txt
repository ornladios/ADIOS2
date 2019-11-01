#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(NUM_TEST_PROCS ${MPIEXEC_MAX_NUMPROCS})
  set(MPIEXEC_COMMAND
    ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${NUM_TEST_PROCS}
  )
  set(test_mpi TRUE)
else()
  set(NUM_TEST_PROCS 1)
  set(test_mpi FALSE)
endif()

include(GoogleTest)
# gtest_add_tests_helper:
# Create a wrapper around gtest_add_tests that uses common patterns for test
# names, execurtable names, mpi usage, etc.
# 
# Arguments:
#   testname - The basename of the test file
#   mpi      - TRUE - build with MPI, execute with mpiexec
#              FALSE - no mpi
#              NOEXEC - build with MPI but don't execute with MPI
#   src_pfx  - Source filename prefix, Test${src_pfs}${testname}.cpp
#   tst_pfx  - Test name prefix to be added to CTest
#   tst_sfx  - Test name suffix to be added to CTest
# all additional arguments are passed directly to gtest_add_tests
#
# Example:
#   You have a gtest file, TestFooThings.cpp that containst Test1 and Test2
#   gtest functions that can be called with different sets of arguments.
#
#     gtest_add_tests_helper(Things TRUE Foo Foo. .Bar EXTRA_ARGS "Bar")
#     gtest_add_tests_helper(Things TRUE Foo Foo. .Baz EXTRA_ARGS "Baz")
#
#  will create the executable Test.Foo.Things and add the tests
#    Foo.Things.Test1.Bar
#    Foo.Things.Test2.Bar
#    Foo.Things.Test1.Baz
#    Foo.Things.Test2.Baz
#
function(gtest_add_tests_helper testname mpi src_pfx tst_pfx tst_sfx)
  set(tgt Test.${tst_pfx}${testname})
  if(NOT TARGET ${tgt})
    add_executable(${tgt} Test${src_pfx}${testname}.cpp)
    target_link_libraries(${tgt} adios2 gtest)
    if(mpi)
      target_link_libraries(${tgt} MPI::MPI_C)
    endif()
  endif()
  if(mpi AND NOT mpi STREQUAL "NOEXEC")
    gtest_add_tests(TARGET ${tgt}
      TEST_PREFIX "${tst_pfx}" TEST_SUFFIX "${tst_sfx}"
      EXEC_WRAPPER ${MPIEXEC_COMMAND} TEST_LIST added_tests
      ${ARGN}
    )
    set_tests_properties(${added_tests} PROPERTIES PROCESSORS ${NUM_TEST_PROCS})
  else()
    gtest_add_tests(TARGET ${tgt}
      TEST_PREFIX "${tst_pfx}" TEST_SUFFIX "${tst_sfx}"
      ${ARGN}
    )
  endif()
endfunction()

add_subdirectory(adios2)
add_subdirectory(utils)
if(ADIOS2_RUN_INSTALL_TEST)
  add_subdirectory(install)
endif()

if(ADIOS2_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
