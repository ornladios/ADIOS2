#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

foreach(tst IN ITEMS
  1D
  2DMemSelect 3DMemSelect
  WriterDoubleBuffer WriterSingleBuffer
  ReaderDoubleBuffer ReaderSingleBuffer
  # Reliable
  )
  gtest_add_tests_helper(${tst} MPI_NONE DataMan Engine.DataMan. "")
  set_tests_properties(${Test.Engine.DataMan.${tst}-TESTS}
    PROPERTIES RUN_SERIAL TRUE
  )
endforeach()

