#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
set(BP3_DIR ./bp3)
file(MAKE_DIRECTORY ${BP3_DIR})

add_test(NAME HeatTransfer.BP3.Write.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp3.xml
    ${BP3_DIR}/HeatTransfer.BP3.Write.MxM.bp 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BP3.Read.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp3.xml
    ${BP3_DIR}/HeatTransfer.BP3.Write.MxM.bp ${BP3_DIR}/HeatTransfer.BP3.Read.MxM.bp 2 2
)
set_property(TEST HeatTransfer.BP3.Read.MxM
  PROPERTY DEPENDS HeatTransfer.BP3.Write.MxM
)

add_test(NAME HeatTransfer.BP3.Dump.MxM
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=${BP3_DIR}/HeatTransfer.BP3.Read.MxM.bp
    -DOUTPUT_FILE=${BP3_DIR}/HeatTransfer.BP3.Dump.MxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BP3.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.BP3.Read.MxM
)

add_test(NAME HeatTransfer.BP3.Validate.MxM
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/${BP3_DIR}/HeatTransfer.BP3.Dump.MxM.txt
)
set_property(TEST HeatTransfer.BP3.Validate.MxM
  PROPERTY DEPENDS HeatTransfer.BP3.Dump.MxM
)

#####################################################################
set(BP4_DIR ./bp4)
file(MAKE_DIRECTORY ${BP4_DIR})

add_test(NAME HeatTransfer.BP4.Write.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4.xml
    ${BP4_DIR}/HeatTransfer.BP4.Write.MxM.bp 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BP4.Read.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4.xml
    ${BP4_DIR}/HeatTransfer.BP4.Write.MxM.bp ${BP4_DIR}/HeatTransfer.BP4.Read.MxM.bp 2 2
)
set_property(TEST HeatTransfer.BP4.Read.MxM
  PROPERTY DEPENDS HeatTransfer.BP4.Write.MxM
)

add_test(NAME HeatTransfer.BP4.Dump.MxM
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=${BP4_DIR}/HeatTransfer.BP4.Read.MxM.bp
    -DOUTPUT_FILE=${BP4_DIR}/HeatTransfer.BP4.Dump.MxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BP4.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.BP4.Read.MxM
)

add_test(NAME HeatTransfer.BP4.Validate.MxM
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/${BP4_DIR}/HeatTransfer.BP4.Dump.MxM.txt
)
set_property(TEST HeatTransfer.BP4.Validate.MxM
  PROPERTY DEPENDS HeatTransfer.BP4.Dump.MxM
)
