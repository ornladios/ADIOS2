#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.SSC.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_ssc.xml
        WriteSSCMxM.bp 2 2 10 10 10 10
    :
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_ssc.xml
        WriteSSCMxM.bp ReadSSCMxM.bp 2 2
)
set_tests_properties(HeatTransfer.SSC.MxM PROPERTIES PROCESSORS 8)

add_test(NAME HeatTransfer.SSC.MxM.Dump
  WORKING_DIRECTORY SSCMxM
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadSSCMxM.bp
    -DOUTPUT_FILE=DumpSSCMxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.SSC.MxM.Validate
  WORKING_DIRECTORY SSCMxM
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpSSCMxM.txt
)

SetupTestPipeline(HeatTransfer.SSC.MxM ";Dump;Validate" TRUE)
