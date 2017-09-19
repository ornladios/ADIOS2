#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestHDF5WriteRead TestHDF5WriteRead.cpp)

# Workaround for multiple versions of FindHDF5
if(HDF5_C_INCLUDE_DIRS)
  target_include_directories(TestHDF5WriteRead PRIVATE ${HDF5_C_INCLUDE_DIRS})
else()
  target_include_directories(TestHDF5WriteRead PRIVATE ${HDF5_INCLUDE_DIRS})
endif()
target_link_libraries(TestHDF5WriteRead adios2 gtest ${HDF5_C_LIBRARIES})

if(ADIOS2_HAVE_MPI)
  target_include_directories(TestHDF5WriteRead PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(TestHDF5WriteRead ${MPI_C_LIBRARIES})
  set(extra_test_args
    EXEC_WRAPPER
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
endif()

gtest_add_tests(TARGET TestHDF5WriteRead ${extra_test_args})
