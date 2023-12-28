#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

set(CTEST_PROJECT_NAME "ADIOS")
set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "open.cdash.org")
set(CTEST_DROP_LOCATION "/submit.php?project=ADIOS")
set(CTEST_DROP_SITE_CDASH TRUE)
set(MEMORYCHECK_SUPPRESSIONS_FILE ${CMAKE_SOURCE_DIR}/scripts/dashboard/nightly/valgrind-suppressions.txt)

# Ignore tests that are currently failing, remove tests here as they are fixed
list(APPEND CTEST_CUSTOM_MEMCHECK_IGNORE 
Engine.BP.BPWriteReadAsStreamTestADIOS2.ReaderWriterDefineVariable.BP5.Serial
Remote.BPWriteReadADIOS2stdio.GetRemote
Remote.BPWriteMemorySelectionRead.GetRemote
Remote.BPWriteMemorySelectionRead.FileRemote
remoteServerCleanup
Engine.SST.SstWriteFails.InvalidBeginStep.Serial
Staging.1x1.Local2.CommMin.BP5.SST
Staging.1x1Struct.CommMin.BP5.SST
Staging.WriteMemorySelectionRead.1x1.CommMin.BP5.SST
Staging.1x1.Local2.CommMin.BP.SST
Staging.WriteMemorySelectionRead.1x1.CommMin.BP.SST
Staging.1x1Struct.BP5
)
