#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.SST.MxN
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.MxN 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.MxN.bp HeatTransfer.SST.Read.MxN 1 3
)

add_test(NAME HeatTransfer.SST.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARGS=-d 
    -DINPUT_FILE=HeatTransfer.SST.Read.MxN.bp
    -DOUTPUT_FILE=HeatTransfer.SST.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/BPLS.cmake"
)
set_property(TEST HeatTransfer.SST.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.SST.Read.MxN
)

add_test(NAME HeatTransfer.SST.Validate.MxN
  COMMAND ${CMAKE_COMMAND}
    -E compare_files
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.SST.Dump.MxN.txt
)
set_property(TEST HeatTransfer.SST.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.SST.Dump.MxN
)
