#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.SST.FFS.MxN
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_ffs.xml
      HeatTransfer.SST.FFS.Write.MxN 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst_ffs.xml
      HeatTransfer.SST.FFS.Write.MxN.bp HeatTransfer.SST.FFS.Read.MxN 1 3
)

add_test(NAME HeatTransfer.SST.FFS.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=HeatTransfer.SST.FFS.Read.MxN.bp
    -DOUTPUT_FILE=HeatTransfer.SST.FFS.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.SST.FFS.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.SST.FFS.Read.MxN
)

add_test(NAME HeatTransfer.SST.FFS.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.SST.FFS.Dump.MxN.txt
)
set_property(TEST HeatTransfer.SST.FFS.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.SST.FFS.Dump.MxN
)
