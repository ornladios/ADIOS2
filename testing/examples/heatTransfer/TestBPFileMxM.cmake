#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# SetupTestPipeline
include(ADIOSFunctions)

add_test(NAME HeatTransfer.BP5.MxM.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp5.xml
        WriteBPFileMxM.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP5.MxM.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP5.MxM.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp5.xml
        WriteBPFileMxM.bp ReadBPFileMxM.bp 2 2
)
set_tests_properties(HeatTransfer.BP5.MxM.Read PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP5.MxM.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadBPFileMxM.bp
    -DOUTPUT_FILE=DumpBPFileMxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP5.MxM.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpBPFileMxM.txt
)

SetupTestPipeline(HeatTransfer.BP5.MxM "Write;Read;Dump;Validate" True)

#####################################################################
add_test(NAME HeatTransfer.BP4.MxM.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp4.xml
        WriteBPFile4MxM.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP4.MxM.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.MxM.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp4.xml
        WriteBPFile4MxM.bp ReadBPFile4MxM.bp 2 2
)
set_tests_properties(HeatTransfer.BP4.MxM.Read PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.MxM.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadBPFile4MxM.bp
    -DOUTPUT_FILE=DumpBPFile4MxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP4.MxM.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpBPFile4MxM.txt
)

SetupTestPipeline(HeatTransfer.BP4.MxM "Write;Read;Dump;Validate" True)
