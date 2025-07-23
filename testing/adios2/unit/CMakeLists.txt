#------------------------------------------------------------------------------#
#Distributed under the OSI - approved Apache License, Version 2.0. See
#accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(ChunkV MPI_NONE "" Unit. "")
gtest_add_tests_helper(CoreDims MPI_NONE "" Unit. "")
if(ADIOS2_HAVE_SST)
  gtest_add_tests_helper(Remote MPI_NONE "" Unit. "" WORKING_DIRECTORY ${REMOTE_DIR})
  set_tests_properties(Unit.Remote.OpenRead.Serial PROPERTIES FIXTURES_REQUIRED Server)
  get_target_property(EVPATH_INCLUDES EVPath::EVPath INCLUDE_DIRECTORIES)
  get_target_property(FFS_INCLUDES ffs::ffs INCLUDE_DIRECTORIES)
  get_target_property(ATL_INCLUDES atl::atl INCLUDE_DIRECTORIES)
  target_include_directories(Test.Unit.Remote.Serial PRIVATE ${EVPATH_INCLUDES};${FFS_INCLUDES};${ATL_INCLUDES})
endif()
if(UNIX)
  gtest_add_tests_helper(PosixTransport MPI_NONE "" Unit. "")
endif()
