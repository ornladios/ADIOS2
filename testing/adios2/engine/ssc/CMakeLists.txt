#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

if(ADIOS2_HAVE_MPI)
  gtest_add_tests_helper(Base MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscBase.MPI "" TRUE)

  gtest_add_tests_helper(OneSidedFencePush MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscOneSidedFencePush.MPI "" TRUE)

  gtest_add_tests_helper(OneSidedPostPush MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscOneSidedPostPush.MPI "" TRUE)

  gtest_add_tests_helper(OneSidedFencePull MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscOneSidedFencePull.MPI "" TRUE)

  gtest_add_tests_helper(OneSidedPostPull MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscOneSidedPostPull.MPI "" TRUE)

  gtest_add_tests_helper(Unbalanced MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscUnbalanced.MPI "" TRUE)

  gtest_add_tests_helper(WriterMultiblock MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscWriterMultiblock.MPI "" TRUE)

  gtest_add_tests_helper(ReaderMultiblock MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscReaderMultiblock.MPI "" TRUE)

  gtest_add_tests_helper(NoAttributes MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscNoAttributes.MPI "" TRUE)

  gtest_add_tests_helper(NoSelection MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscNoSelection.MPI "" TRUE)

  gtest_add_tests_helper(7d MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSsc7d.MPI "" TRUE)

  gtest_add_tests_helper(MoreReadersThanWriters MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscMoreReadersThanWriters.MPI "" TRUE)

  gtest_add_tests_helper(MoreWritersThanReaders MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscMoreWritersThanReaders.MPI "" TRUE)

  gtest_add_tests_helper(MultiApp MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscMultiApp.MPI "" TRUE)

  gtest_add_tests_helper(Xgc2Way MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscXgc2Way.MPI "" TRUE)

  gtest_add_tests_helper(Xgc3Way MPI_ONLY Ssc Engine.SSC. "")
  SetupTestPipeline(Engine.SSC.SscEngineTest.TestSscXgc3Way.MPI "" TRUE)

endif()
