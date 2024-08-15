#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(ADIOSFunctions)

add_test(NAME HeatTransfer.InsituMPI.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_EXTRA_FLAGS}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:adios2_simulations_heatTransferWrite>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_ssc.xml
        WriteInsituMx1.bp 2 2 10 10 10 10
    :
    ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:adios2_simulations_heatTransferRead>
        ${PROJECT_SOURCE_DIR}/examples/simulations/heatTransfer/heat_ssc.xml
        WriteInsituMx1.bp ReadInsituMx1.bp 1 1
)
set_tests_properties(HeatTransfer.InsituMPI.Mx1 PROPERTIES PROCESSORS 5)

add_test(NAME HeatTransfer.InsituMPI.Mx1.Dump
  WORKING_DIRECTORY InsituMx1
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=ReadInsituMx1.bp
    -DOUTPUT_FILE=DumpInsituMx1.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME HeatTransfer.InsituMPI.Mx1.Validate
  WORKING_DIRECTORY InsituMx1
  COMMAND ${DIFF_COMMAND} -u -w
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    DumpInsituMx1.txt
)

SetupTestPipeline(HeatTransfer.InsituMPI.Mx1 ";Dump;Validate" TRUE)
