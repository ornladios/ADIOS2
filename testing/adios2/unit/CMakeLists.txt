#------------------------------------------------------------------------------#
#Distributed under the OSI - approved Apache License, Version 2.0. See
#accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(ChunkV MPI_NONE "" Unit. "")
gtest_add_tests_helper(CoreDims MPI_NONE "" Unit. "")
if(ADIOS2_HAVE_SST)
  gtest_add_tests_helper(Remote MPI_NONE "" Unit. "" WORKING_DIRECTORY ${REMOTE_DIR})
  set_tests_properties(Unit.Remote.OpenRead.Serial PROPERTIES FIXTURES_REQUIRED Server)
  get_target_property(EVPATH_INCLUDES adios2::thirdparty::EVPath INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(FFS_INCLUDES adios2::thirdparty::ffs INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(ATL_INCLUDES adios2::thirdparty::atl INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(Test.Unit.Remote.Serial PRIVATE ${EVPATH_INCLUDES};${FFS_INCLUDES};${ATL_INCLUDES})
endif()
if(UNIX)
  gtest_add_tests_helper(PosixTransport MPI_NONE "" Unit. "")
endif()
gtest_add_tests_helper(FilePool MPI_NONE "" Unit. "")

# AWS SDK S3 transport tests - disabled by default, require S3-compatible endpoint
# Run manually with: ./bin/Test.Unit.AWSSDKTransport.Serial --gtest_also_run_disabled_tests
if(ADIOS2_HAVE_AWSSDK)
  gtest_add_tests_helper(AWSSDKTransport MPI_NONE "" Unit. "")
  # Link to AWS SDK and aws-crt-cpp to get all includes
  find_package(aws-crt-cpp QUIET)
  if(aws-crt-cpp_FOUND)
    target_link_libraries(Test.Unit.AWSSDKTransport.Serial AWS::aws-crt-cpp)
  endif()
  target_link_libraries(Test.Unit.AWSSDKTransport.Serial aws-cpp-sdk-s3 aws-cpp-sdk-core)
endif()
