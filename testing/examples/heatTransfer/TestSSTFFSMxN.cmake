#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.SST.FFS.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_sst_ffs.xml
        Write.bp 2 2 10 10 10 10 SST
    :
    ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:heatTransfer_read>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_sst_ffs.xml
        Write.bp Read.bp 1 3 SST
)
set_tests_properties(HeatTransfer.SST.FFS.MxN PROPERTIES PROCESSORS 7)

add_test(NAME HeatTransfer.SST.FFS.MxN.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=Read.bp
    -DOUTPUT_FILE=Dump.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.SST.FFS.MxN.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    Dump.txt
)

SetupTestPipeline(HeatTransfer.SST.FFS.MxN ";Dump;Validate" TRUE)
