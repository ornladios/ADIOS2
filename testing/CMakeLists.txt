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
function(gtest_add_tests_helper testname mpi pfx sfx)
  set(tgt Test.${pfx}${testname})
  if(NOT TARGET ${tgt})
    add_executable(${tgt} Test${testname}.cpp)
    target_link_libraries(${tgt} adios2 gtest)
    if(mpi)
      target_link_libraries(${tgt} MPI::MPI_C)
    endif()
  endif()
  if(mpi)
    gtest_add_tests(TARGET ${tgt} TEST_PREFIX "${pfx}" TEST_SUFFIX "${sfx}"
      EXEC_WRAPPER ${MPIEXEC_COMMAND} TEST_LIST added_tests
      ${ARGN}
    )
    set_tests_properties(${added_tests} PROPERTIES PROCESSORS ${NUM_TEST_PROCS})
  else()
    gtest_add_tests(TARGET ${tgt} TEST_PREFIX "${pfx}" TEST_SUFFIX "${sfx}"
      ${ARGN}
    )
  endif()
endfunction()

add_subdirectory(adios2)
add_subdirectory(utils)
add_subdirectory(install)

if(ADIOS2_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
