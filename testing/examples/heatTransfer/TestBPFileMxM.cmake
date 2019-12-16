#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# SetupTestPipeline
include(ADIOSFunctions)

add_test(NAME HeatTransfer.BP3.MxM.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp3.xml
        Write.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP3.MxM.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP3.MxM.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_read>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp3.xml
        Write.bp Read.bp 2 2
)
set_tests_properties(HeatTransfer.BP3.MxM.Read PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP3.MxM.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=Read.bp
    -DOUTPUT_FILE=Dump.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP3.MxM.Validate
  COMMAND ${DIFF_COMMAND} -uw
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    Dump.txt
)

SetupTestPipeline(HeatTransfer.BP3.MxM "Write;Read;Dump;Validate" True)

#####################################################################
add_test(NAME HeatTransfer.BP4.MxM.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp4.xml
        Write.bp 2 2 10 10 10 10
)
set_tests_properties(HeatTransfer.BP4.MxM.Write PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.MxM.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_read>
        ${PROJECT_SOURCE_DIR}/examples/heatTransfer/heat_bp4.xml
        Write.bp Read.bp 2 2
)
set_tests_properties(HeatTransfer.BP4.MxM.Read PROPERTIES PROCESSORS 4)

add_test(NAME HeatTransfer.BP4.MxM.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=Read.bp
    -DOUTPUT_FILE=Dump.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.BP4.MxM.Validate
  COMMAND ${DIFF_COMMAND} -uw
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    Dump.txt
)

SetupTestPipeline(HeatTransfer.BP4.MxM "Write;Read;Dump;Validate" True)
