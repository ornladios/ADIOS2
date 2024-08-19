#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.SST.BP.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_sst_bp.xml
        WriteSSTBPMxN.bp 2 2 10 10 10 10 SST
    :
    ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_sst_bp.xml
        WriteSSTBPMxN.bp ReadSSTBPMxN.bp 1 3 SST
)
set_tests_properties(HeatTransfer.SST.BP.MxN PROPERTIES PROCESSORS 7)

add_test(NAME HeatTransfer.SST.BP.MxN.Dump
  WORKING_DIRECTORY SSTBPMxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadSSTBPMxN.bp
    -DOUTPUT_FILE=DumpSSTBPMxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.SST.BP.MxN.Validate
  WORKING_DIRECTORY SSTBPMxN
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpSSTBPMxN.txt
)

SetupTestPipeline(HeatTransfer.SST.BP.MxN ";Dump;Validate" TRUE)
