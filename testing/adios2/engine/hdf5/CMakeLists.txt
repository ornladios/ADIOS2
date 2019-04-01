#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(TestNativeHDF5WriteRead TestNativeHDF5WriteRead.cpp)
add_executable(TestHDF5WriteReadAsStream TestHDF5WriteReadAsStream.cpp)
add_executable(TestStreamWriteReadHighLevelAPI_HDF5 TestStreamWriteReadHighLevelAPI_HDF5.cpp)
add_executable(TestHDF5WriteReadAttributesADIOS2 TestHDF5WriteReadAttributesADIOS2.cpp)	

# Workaround for multiple versions of FindHDF5
if(HDF5_C_INCLUDE_DIRS)
  target_include_directories(TestNativeHDF5WriteRead PRIVATE ${HDF5_C_INCLUDE_DIRS})
else()
  target_include_directories(TestNativeHDF5WriteRead PRIVATE ${HDF5_INCLUDE_DIRS})
endif()
target_link_libraries(TestNativeHDF5WriteRead adios2 gtest_interface ${HDF5_C_LIBRARIES})
target_link_libraries(TestHDF5WriteReadAsStream adios2 gtest_interface ${HDF5_C_LIBRARIES})
target_link_libraries(TestStreamWriteReadHighLevelAPI_HDF5 adios2 gtest_interface ${HDF5_C_LIBRARIES})
target_link_libraries(TestHDF5WriteReadAttributesADIOS2 adios2 gtest_interface ${HDF5_C_LIBRARIES})

if(ADIOS2_HAVE_MPI)
  target_link_libraries(TestNativeHDF5WriteRead MPI::MPI_C)
  target_link_libraries(TestHDF5WriteReadAsStream MPI::MPI_C)
  target_link_libraries(TestStreamWriteReadHighLevelAPI_HDF5 MPI::MPI_C)
  target_link_libraries(TestHDF5WriteReadAttributesADIOS2 MPI::MPI_C)
  set(extra_test_args EXEC_WRAPPER ${MPIEXEC_COMMAND})
endif()

gtest_add_tests(TARGET TestNativeHDF5WriteRead ${extra_test_args})
gtest_add_tests(TARGET TestHDF5WriteReadAsStream ${extra_test_args})
gtest_add_tests(TARGET TestStreamWriteReadHighLevelAPI_HDF5 ${extra_test_args})
gtest_add_tests(TARGET TestHDF5WriteReadAttributesADIOS2 ${extra_test_args})
