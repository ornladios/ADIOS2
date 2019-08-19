#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.BPFile.Write.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.MxM 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BPFile.Read.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.MxM.bp HeatTransfer.BPFile.Read.MxM 2 2
)
set_property(TEST HeatTransfer.BPFile.Read.MxM
  PROPERTY DEPENDS HeatTransfer.BPFile.Write.MxM
)

add_test(NAME HeatTransfer.BPFile.Dump.MxM
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=HeatTransfer.BPFile.Read.MxM.bp
    -DOUTPUT_FILE=HeatTransfer.BPFile.Dump.MxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BPFile.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.BPFile.Read.MxM
)

add_test(NAME HeatTransfer.BPFile.Validate.MxM
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.BPFile.Dump.MxM.txt
)
set_property(TEST HeatTransfer.BPFile.Validate.MxM
  PROPERTY DEPENDS HeatTransfer.BPFile.Dump.MxM
)

#####################################################################
set(BP4_DIR ./bp4)
file(MAKE_DIRECTORY ${BP4_DIR})

add_test(NAME HeatTransfer.BP4File.Write.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4file.xml
    ${BP4_DIR}/HeatTransfer.BP4File.Write.MxM 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BP4File.Read.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4file.xml
    ${BP4_DIR}/HeatTransfer.BP4File.Write.MxM.bp ${BP4_DIR}/HeatTransfer.BP4File.Read.MxM 2 2
)
set_property(TEST HeatTransfer.BP4File.Read.MxM
  PROPERTY DEPENDS HeatTransfer.BP4File.Write.MxM
)

add_test(NAME HeatTransfer.BP4File.Dump.MxM
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=${BP4_DIR}/HeatTransfer.BP4File.Read.MxM.bp
    -DOUTPUT_FILE=${BP4_DIR}/HeatTransfer.BP4File.Dump.MxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BP4File.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.BP4File.Read.MxM
)

add_test(NAME HeatTransfer.BP4File.Validate.MxM
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/${BP4_DIR}/HeatTransfer.BP4File.Dump.MxM.txt
)
set_property(TEST HeatTransfer.BP4File.Validate.MxM
  PROPERTY DEPENDS HeatTransfer.BP4File.Dump.MxM
)
