#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.BPFile.Write.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.Mx1 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BPFile.Read.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.Mx1.bp HeatTransfer.BPFile.Read.Mx1 1 1
)
set_property(TEST HeatTransfer.BPFile.Read.Mx1
  PROPERTY DEPENDS HeatTransfer.BPFile.Write.Mx1
)

add_test(NAME HeatTransfer.BPFile.Dump.Mx1
  COMMAND ${CMAKE_COMMAND}
    -DARGS=-d 
    -DINPUT_FILE=HeatTransfer.BPFile.Read.Mx1.bp
    -DOUTPUT_FILE=HeatTransfer.BPFile.Dump.Mx1.txt
    -P "${PROJECT_BINARY_DIR}/bpls2.cmake"
)
set_property(TEST HeatTransfer.BPFile.Dump.Mx1
  PROPERTY DEPENDS HeatTransfer.BPFile.Read.Mx1
)

add_test(NAME HeatTransfer.BPFile.Validate.Mx1
  COMMAND ${CMAKE_COMMAND}
    -E compare_files
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.BPFile.Dump.Mx1.txt
)
set_property(TEST HeatTransfer.BPFile.Validate.Mx1
  PROPERTY DEPENDS HeatTransfer.BPFile.Dump.Mx1
)
