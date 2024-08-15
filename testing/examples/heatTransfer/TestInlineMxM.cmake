#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.Inline.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferInline>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_inline.xml
        ReadInlineMxN.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.Inline.MxM PROPERTIES PROCESSORS 8)

add_test(NAME HeatTransfer.Inline.MxM.Dump
  WORKING_DIRECTORY InlineMxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadInlineMxN.bp
    -DOUTPUT_FILE=DumpInlineMxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.Inline.MxM.Validate
  WORKING_DIRECTORY InlineMxN
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpInlineMxN.txt
)

SetupTestPipeline(HeatTransfer.Inline.MxM ";Dump;Validate" TRUE)
