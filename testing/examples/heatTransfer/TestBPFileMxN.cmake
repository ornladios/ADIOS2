#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# SetupTestPipeline
include(ADIOSFunctions)

add_test(NAME HeatTransfer.BP5.MxN.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp5.xml
        WriteBPFileMxN.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP5.MxN.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP5.MxN.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp5.xml
        WriteBPFileMxN.bp ReadBPFileMxN.bp 1 3
)
set_tests_properties(HeatTransfer.BP5.MxN.Read PROPERTIES PROCESSORS 3)

add_test(NAME HeatTransfer.BP5.MxN.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadBPFileMxN.bp
    -DOUTPUT_FILE=DumpBPFileMxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP5.MxN.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpBPFileMxN.txt
)

SetupTestPipeline(HeatTransfer.BP5.MxN "Write;Read;Dump;Validate" True)

#####################################################################
add_test(NAME HeatTransfer.BP4.MxN.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp4.xml
        WriteBPFile4MxN.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP4.MxN.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.MxN.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp4.xml
        WriteBPFile4MxN.bp ReadBPFile4MxN.bp 1 3
)
set_tests_properties(HeatTransfer.BP4.MxN.Read PROPERTIES PROCESSORS 3)

add_test(NAME HeatTransfer.BP4.MxN.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadBPFile4MxN.bp
    -DOUTPUT_FILE=DumpBPFile4MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP4.MxN.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpBPFile4MxN.txt
)

SetupTestPipeline(HeatTransfer.BP4.MxN "Write;Read;Dump;Validate" True)
