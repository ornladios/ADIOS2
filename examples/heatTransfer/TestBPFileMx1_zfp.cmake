#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.BPFile.Write.Mx1_zfp
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile_zfp.xml
    HeatTransfer.BPFile.Write.Mx1_zfp 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BPFile.Read.Mx1_zfp
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.Mx1_zfp.bp HeatTransfer.BPFile.Read.Mx1_zfp 1 1
)
set_property(TEST HeatTransfer.BPFile.Read.Mx1_zfp
  PROPERTY DEPENDS HeatTransfer.BPFile.Write.Mx1_zfp
)

add_test(NAME HeatTransfer.BPFile.Dump.Mx1_zfp
  COMMAND ${CMAKE_COMMAND}
    -DARGS=-d 
    -DINPUT_FILE=HeatTransfer.BPFile.Read.Mx1_zfp.bp
    -DOUTPUT_FILE=HeatTransfer.BPFile.Dump.Mx1_zfp.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BPFile.Dump.Mx1_zfp
  PROPERTY DEPENDS HeatTransfer.BPFile.Read.Mx1_zfp
)

# TODO: find a validate method to a certain accuracy for lossy compression
# add_test(NAME HeatTransfer.BPFile.Validate.Mx1_zfp
#  COMMAND ${CMAKE_COMMAND}
#    -E compare_files
#    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
#    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.BPFile.Dump.Mx1_zfp.txt
#)
#set_property(TEST HeatTransfer.BPFile.Validate.Mx1_zfp
#  PROPERTY DEPENDS HeatTransfer.BPFile.Dump.Mx1_zfp
#)
