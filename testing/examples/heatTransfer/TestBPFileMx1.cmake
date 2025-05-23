#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.BP5.Mx1.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp5.xml
        WriteBPFileMx1.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP5.Mx1.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP5.Mx1.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp5.xml
        WriteBPFileMx1.bp ReadBPFileMx1.bp 1 1
)

add_test(NAME HeatTransfer.BP5.Mx1.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadBPFileMx1.bp
    -DOUTPUT_FILE=DumpBPFileMx1.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP5.Mx1.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpBPFileMx1.txt
)

SetupTestPipeline(HeatTransfer.BP5.Mx1 "Write;Read;Dump;Validate" TRUE)

#############################################################################

add_test(NAME HeatTransfer.BP4.Mx1.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp4.xml
        WriteBPFileMx1.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP4.Mx1.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.Mx1.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_bp4.xml
        WriteBPFileMx1.bp ReadBPFileMx1.bp 1 1
)

add_test(NAME HeatTransfer.BP4.Mx1.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadBPFileMx1.bp
    -DOUTPUT_FILE=DumpBPFileMx1.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP4.Mx1.Validate
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpBPFileMx1.txt
)

SetupTestPipeline(HeatTransfer.BP4.Mx1 "Write;Read;Dump;Validate" TRUE)
