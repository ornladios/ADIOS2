#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(WriteReadAsStream ${test_mpi} HDF5 Engine.HDF5. "")
gtest_add_tests_helper(StreamWriteReadHighLevelAPI ${test_mpi}
  HDF5 Engine.HDF5. ""
)
gtest_add_tests_helper(WriteReadAttributesADIOS2 ${test_mpi}
  HDF5 Engine.HDF5. ""
)
gtest_add_tests_helper(WriteMemorySelectionRead  ${test_mpi}
  HDF5 Engine.HDF5. ""
)

gtest_add_tests_helper(NativeHDF5WriteRead ${test_mpi} "" Engine.HDF5. "")
if(HDF5_C_INCLUDE_DIRS)
  target_include_directories(Test.Engine.HDF5.NativeHDF5WriteRead
    PRIVATE ${HDF5_C_INCLUDE_DIRS}
  )
else()
  target_include_directories(Test.Engine.HDF5.NativeHDF5WriteRead
    PRIVATE ${HDF5_INCLUDE_DIRS}
  )
endif()
target_link_libraries(Test.Engine.HDF5.NativeHDF5WriteRead ${HDF5_C_LIBRARIES})
