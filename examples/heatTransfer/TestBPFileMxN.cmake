#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.BPFile.Write.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.MxN 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BPFile.Read.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 3
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.MxN.bp HeatTransfer.BPFile.Read.MxN 1 3
)
set_property(TEST HeatTransfer.BPFile.Read.MxN
  PROPERTY DEPENDS HeatTransfer.BPFile.Write.MxN
)

add_test(NAME HeatTransfer.BPFile.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=HeatTransfer.BPFile.Read.MxN.bp
    -DOUTPUT_FILE=HeatTransfer.BPFile.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BPFile.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.BPFile.Read.MxN
)

add_test(NAME HeatTransfer.BPFile.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.BPFile.Dump.MxN.txt
)
set_property(TEST HeatTransfer.BPFile.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.BPFile.Dump.MxN
)


#####################################################################
set(BP4_DIR ./bp4)
file(MAKE_DIRECTORY ${BP4_DIR})

add_test(NAME HeatTransfer.BP4File.Write.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4file.xml
    ${BP4_DIR}/HeatTransfer.BP4File.Write.MxN 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BP4File.Read.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 3
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4file.xml
    ${BP4_DIR}/HeatTransfer.BP4File.Write.MxN.bp ${BP4_DIR}/HeatTransfer.BP4File.Read.MxN 1 3
)
set_property(TEST HeatTransfer.BP4File.Read.MxN
  PROPERTY DEPENDS HeatTransfer.BP4File.Write.MxN
)

add_test(NAME HeatTransfer.BP4File.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=${BP4_DIR}/HeatTransfer.BP4File.Read.MxN.bp
    -DOUTPUT_FILE=${BP4_DIR}/HeatTransfer.BP4File.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BP4File.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.BP4File.Read.MxN
)

add_test(NAME HeatTransfer.BP4File.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/${BP4_DIR}/HeatTransfer.BP4File.Dump.MxN.txt
)
set_property(TEST HeatTransfer.BP4File.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.BP4File.Dump.MxN
)
