#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(Base TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscBase "" TRUE)

  gtest_add_tests_helper(BaseOneSidedFencePush TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscBaseOneSidedFencePush "" TRUE)

  gtest_add_tests_helper(BaseOneSidedPostPush TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscBaseOneSidedPostPush "" TRUE)

  gtest_add_tests_helper(Unbalanced TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscUnbalanced "" TRUE)

  gtest_add_tests_helper(WriterMultiblock TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscWriterMultiblock "" TRUE)

  gtest_add_tests_helper(NoAttributes TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscNoAttributes "" TRUE)

  gtest_add_tests_helper(NoSelection TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscNoSelection "" TRUE)

  gtest_add_tests_helper(7d TRUE Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSsc7d "" TRUE)
endif()
