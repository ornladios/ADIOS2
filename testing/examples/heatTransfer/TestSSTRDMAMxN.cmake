#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.SST.RDMA.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_sst_rdma.xml
        WriteSSTRDMAMxN.bp 2 2 10 10 10 10 SST
    :
    ${MPIEXEC_NUMPROC_FLAG} 5
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_sst_rdma.xml
        WriteSSTRDMAMxN.bp ReadSSTRDMAMxN.bp 1 5 SST
)
set_tests_properties(HeatTransfer.SST.RDMA.MxN PROPERTIES PROCESSORS 7)

add_test(NAME HeatTransfer.SST.RDMA.MxN.Dump
  WORKING_DIRECTORY SSTRDMAMxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadSSTRDMAMxN.bp
    -DOUTPUT_FILE=DumpSSTRDMAMxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.SST.RDMA.MxN.Validate
  WORKING_DIRECTORY SSTRDMAMxN
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpSSTRDMAMxN.txt
)

SetupTestPipeline(HeatTransfer.SST.RDMA.MxN ";Dump;Validate" TRUE)
