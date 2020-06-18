#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  set(MPIEXEC_EXTRA_FLAGS "" CACHE STRING
      "Extra flags to set after mpiexec and before the num_procs flag"
  )
  mark_as_advanced(MPIEXEC_EXTRA_FLAGS)
  separate_arguments(MPIEXEC_EXTRA_FLAGS)

  set(MPIEXEC_COMMAND
    ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
endif()

include(GoogleTest)
# gtest_add_tests_helper:
# Create a wrapper around gtest_add_tests that uses common patterns for test
# names, execurtable names, mpi usage, etc.
#
# Arguments:
#   testname - The basename of the test file
#   mpi      - MPI_ALLOW - build MPI test, execute with mpiexec; also build Serial test
#              MPI_NONE - build Serial test only
#              MPI_ONLY - build MPI test only, execute with mpiexec
#   src_pfx  - Source filename prefix, Test${src_pfs}${testname}.cpp
#   tst_pfx  - Test name prefix to be added to CTest
#   tst_sfx  - Test name suffix to be added to CTest
# all additional arguments are passed directly to gtest_add_tests
#
# Example:
#   You have a gtest file, TestFooThings.cpp that containst Test1 and Test2
#   gtest functions that can be called with different sets of arguments.
#
#     gtest_add_tests_helper(Things MPI_ALLOW Foo Foo. .Bar EXTRA_ARGS "Bar")
#     gtest_add_tests_helper(Things MPI_ALLOW Foo Foo. .Baz EXTRA_ARGS "Baz")
#
#  will create the executable Test.Foo.Things and add the tests
#    Foo.Things.Test1.Bar.Serial
#    Foo.Things.Test1.Bar.MPI
#    Foo.Things.Test2.Bar.Serial
#    Foo.Things.Test2.Bar.MPI
#    Foo.Things.Test1.Baz.Serial
#    Foo.Things.Test1.Baz.MPI
#    Foo.Things.Test2.Baz.Serial
#    Foo.Things.Test2.Baz.MPI
#
function(gtest_add_tests_helper testname mpi src_pfx tst_pfx tst_sfx)
  set(test_targets "")
  if(NOT mpi MATCHES "^MPI_(ALLOW|NONE|ONLY)$")
    message(FATAL_ERROR "Invalid mpi argument value '${mpi}'.")
  endif()
  set(all_tests)
  if(NOT mpi STREQUAL "MPI_ONLY")
    set(tgt Test.${tst_pfx}${testname}.Serial)
    list(APPEND test_targets "${tgt}")
    if(NOT TARGET ${tgt})
      add_executable(${tgt} Test${src_pfx}${testname}.cpp)
      target_link_libraries(${tgt} adios2::cxx11 adios2::c adios2_core adios2::thirdparty::gtest)
    endif()
    gtest_add_tests(TARGET ${tgt}
      TEST_PREFIX "${tst_pfx}"
      TEST_SUFFIX "${tst_sfx}.Serial"
      TEST_LIST added_tests
      ${ARGN}
    )
    list(APPEND all_tests ${added_tests})
  endif()
  if(ADIOS2_HAVE_MPI AND NOT mpi STREQUAL "MPI_NONE")
    set(tgt Test.${tst_pfx}${testname}.MPI)
    list(APPEND test_targets "${tgt}")
    if(NOT TARGET ${tgt})
      add_executable(${tgt} Test${src_pfx}${testname}.cpp)
      target_link_libraries(${tgt} adios2::cxx11_mpi adios2::c_mpi adios2_core_mpi MPI::MPI_C adios2::thirdparty::gtest)
    endif()
    gtest_add_tests(TARGET ${tgt}
      TEST_PREFIX "${tst_pfx}"
      TEST_SUFFIX "${tst_sfx}.MPI"
      EXEC_WRAPPER ${MPIEXEC_COMMAND}
      TEST_LIST added_tests
      ${ARGN}
    )
    list(APPEND all_tests ${added_tests})
    set_tests_properties(${added_tests} PROPERTIES PROCESSORS "${MPIEXEC_MAX_NUMPROCS}")
  endif()
  set("Test.${tst_pfx}${testname}-TESTS" "${all_tests}" PARENT_SCOPE)
  set("Test.${tst_pfx}${testname}-TARGETS" "${test_targets}" PARENT_SCOPE)
endfunction()

add_subdirectory(adios2)
add_subdirectory(utils)
if(ADIOS2_RUN_INSTALL_TEST)
  add_subdirectory(install)
endif()

if(ADIOS2_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
