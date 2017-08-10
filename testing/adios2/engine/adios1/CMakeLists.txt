#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT ADIOS2_HAVE_MPI)
  find_package(ADIOS1 COMPONENTS sequential REQUIRED)
else()
  find_package(ADIOS1 REQUIRED)
endif()

add_executable(TestADIOS1WriteRead TestADIOS1WriteRead.cpp)
target_link_libraries(TestADIOS1WriteRead adios2 gtest adios1::adios)

if(NOT ADIOS2_HAVE_MPI)
  gtest_add_tests(TARGET TestADIOS1WriteRead)
else()
  # MPI versions of the test
  gtest_add_tests(TARGET TestADIOS1WriteRead EXEC_WRAPPER ${MPIEXEC}
                                                        ${MPIEXEC_NUMPROC_FLAG}
                                                        ${MPIEXEC_MAX_NUMPROCS})
endif()
