#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.SST.BP.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_bp.xml
      HeatTransfer.SST.BP.Write.Mx1 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_bp.xml
      HeatTransfer.SST.BP.Write.Mx1.bp HeatTransfer.SST.BP.Read.Mx1 1 1
)

add_test(NAME HeatTransfer.SST.BP.Dump.Mx1
  COMMAND ${CMAKE_COMMAND}
    -DARGS=-d 
    -DINPUT_FILE=HeatTransfer.SST.BP.Read.Mx1.bp
    -DOUTPUT_FILE=HeatTransfer.SST.BP.Dump.Mx1.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.SST.BP.Dump.Mx1
  PROPERTY DEPENDS HeatTransfer.SST.BP.Read.Mx1
)

add_test(NAME HeatTransfer.SST.BP.Validate.Mx1
  COMMAND ${CMAKE_COMMAND}
    -E compare_files
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.SST.BP.Dump.Mx1.txt
)
set_property(TEST HeatTransfer.SST.BP.Validate.Mx1
  PROPERTY DEPENDS HeatTransfer.SST.BP.Dump.Mx1
)
