#------------------------------------------------------------------------------#
#Distributed under the OSI - approved Apache License, Version 2.0. See
#accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

gtest_add_tests_helper(ChunkV MPI_NONE "" Unit. "")
gtest_add_tests_helper(CoreDims MPI_NONE "" Unit. "")
gtest_add_tests_helper(Remote MPI_NONE "" Unit. "")
set_tests_properties(Unit.Remote.OpenRead.Serial PROPERTIES FIXTURES_REQUIRED Server)
target_link_libraries(Test.Unit.Remote.Serial adios2::thirdparty::EVPath)
if(UNIX)
  gtest_add_tests_helper(PosixTransport MPI_NONE "" Unit. "")
endif()

