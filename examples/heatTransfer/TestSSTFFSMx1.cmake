#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.SST.FFS.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_ffs.xml
      HeatTransfer.SST.FFS.Write.Mx1 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_ffs.xml
      HeatTransfer.SST.FFS.Write.Mx1.bp HeatTransfer.SST.FFS.Read.Mx1 1 1
)

add_test(NAME HeatTransfer.SST.FFS.Dump.Mx1
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=HeatTransfer.SST.FFS.Read.Mx1.bp
    -DOUTPUT_FILE=HeatTransfer.SST.FFS.Dump.Mx1.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.SST.FFS.Dump.Mx1
  PROPERTY DEPENDS HeatTransfer.SST.FFS.Read.Mx1
)

add_test(NAME HeatTransfer.SST.FFS.Validate.Mx1
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.SST.FFS.Dump.Mx1.txt
)
set_property(TEST HeatTransfer.SST.FFS.Validate.Mx1
  PROPERTY DEPENDS HeatTransfer.SST.FFS.Dump.Mx1
)
