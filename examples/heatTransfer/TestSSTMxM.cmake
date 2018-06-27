#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.SST.MxM
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.MxM 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.MxM.bp HeatTransfer.SST.Read.MxM 2 2
)

add_test(NAME HeatTransfer.SST.Dump.MxM
  COMMAND ${CMAKE_COMMAND}
    -DARGS=-d 
    -DINPUT_FILE=HeatTransfer.SST.Read.MxM.bp
    -DOUTPUT_FILE=HeatTransfer.SST.Dump.MxM.txt
    -P "${PROJECT_BINARY_DIR}/bpls2.cmake"
)
set_property(TEST HeatTransfer.SST.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.SST.Read.MxM
)

add_test(NAME HeatTransfer.SST.Validate.MxM
  COMMAND ${CMAKE_COMMAND}
    -E compare_files
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.SST.Dump.MxM.txt
)
set_property(TEST HeatTransfer.SST.Validate.MxM
  PROPERTY DEPENDS HeatTransfer.SST.Dump.MxM
)
