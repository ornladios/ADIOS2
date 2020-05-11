#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(Threads REQUIRED)

add_executable(Test.Engine.Common.MPI TestEngineCommon.cpp)
target_link_libraries(Test.Engine.Common.MPI PRIVATE
  adios2::cxx11_mpi
  MPI::MPI_C
  adios2::thirdparty::gtest
  ${CMAKE_THREAD_LIBS_INIT}
)

# Note: extra arguments
#   1st arg: Engine
#   2nd arg: 1 for serialized execution, 0 for concurrent execution of Writer/Reader
#   3rd arg: engine parameters

gtest_add_tests_helper(Common MPI_ONLY "" Engine. .File
  EXTRA_ARGS "File" "1"
)
                
if(ADIOS2_HAVE_HDF5 AND HDF5_IS_PARALLEL)
  gtest_add_tests_helper(Common MPI_ONLY "" Engine. .HDF5
    EXTRA_ARGS "HDF5" "1"
  )
endif()

if(ADIOS2_HAVE_SST)
  gtest_add_tests_helper(Common MPI_ONLY "" Engine. .SST.FFS
    EXTRA_ARGS "SST" "0" "MarshalMethod:FFS"
  )
  gtest_add_tests_helper(Common MPI_ONLY "" Engine. .SST.BP
    EXTRA_ARGS "SST" "0" "MarshalMethod:BP"
  )
endif()

gtest_add_tests_helper(Common MPI_ONLY "" Engine. .InSituMPI
  EXTRA_ARGS "InSituMPI" "0"
)

#if(ADIOS2_HAVE_DataMan)
#  gtest_add_tests_helper(Common MPI_ONLY "" Engine. .DataMan
#    EXTRA_ARGS "DataMan" "0"
#  )
#endif()
