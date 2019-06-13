#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.SST.BP.MxN
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_bp.xml
      HeatTransfer.SST.BP.Write.MxN 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_bp.xml
      HeatTransfer.SST.BP.Write.MxN.bp HeatTransfer.SST.BP.Read.MxN 1 3
)

add_test(NAME HeatTransfer.SST.BP.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARGS=-d 
    -DINPUT_FILE=HeatTransfer.SST.BP.Read.MxN.bp
    -DOUTPUT_FILE=HeatTransfer.SST.BP.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.SST.BP.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.SST.BP.Read.MxN
)

add_test(NAME HeatTransfer.SST.BP.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.SST.BP.Dump.MxN.txt
)
set_property(TEST HeatTransfer.SST.BP.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.SST.BP.Dump.MxN
)
