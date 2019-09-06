#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
set(BP3_DIR ./bp3)
file(MAKE_DIRECTORY ${BP3_DIR})

add_test(NAME HeatTransfer.BP3.Write.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp3.xml
    ${BP3_DIR}/HeatTransfer.BP3.Write.MxN.bp 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BP3.Read.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 3
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp3.xml
    ${BP3_DIR}/HeatTransfer.BP3.Write.MxN.bp ${BP3_DIR}/HeatTransfer.BP3.Read.MxN.bp 1 3
)
set_property(TEST HeatTransfer.BP3.Read.MxN
  PROPERTY DEPENDS HeatTransfer.BP3.Write.MxN
)

add_test(NAME HeatTransfer.BP3.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=${BP3_DIR}/HeatTransfer.BP3.Read.MxN.bp
    -DOUTPUT_FILE=${BP3_DIR}/HeatTransfer.BP3.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BP3.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.BP3.Read.MxN
)

add_test(NAME HeatTransfer.BP3.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/${BP3_DIR}/HeatTransfer.BP3.Dump.MxN.txt
)
set_property(TEST HeatTransfer.BP3.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.BP3.Dump.MxN
)


#####################################################################
set(BP4_DIR ./bp4)
file(MAKE_DIRECTORY ${BP4_DIR})

add_test(NAME HeatTransfer.BP4.Write.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4.xml
    ${BP4_DIR}/HeatTransfer.BP4.Write.MxN.bp 2 2 10 10 10 10
)

add_test(NAME HeatTransfer.BP4.Read.MxN
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 3
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bp4.xml
    ${BP4_DIR}/HeatTransfer.BP4.Write.MxN.bp ${BP4_DIR}/HeatTransfer.BP4.Read.MxN.bp 1 3
)
set_property(TEST HeatTransfer.BP4.Read.MxN
  PROPERTY DEPENDS HeatTransfer.BP4.Write.MxN
)

add_test(NAME HeatTransfer.BP4.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=${BP4_DIR}/HeatTransfer.BP4.Read.MxN.bp
    -DOUTPUT_FILE=${BP4_DIR}/HeatTransfer.BP4.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.BP4.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.BP4.Read.MxN
)

add_test(NAME HeatTransfer.BP4.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/${BP4_DIR}/HeatTransfer.BP4.Dump.MxN.txt
)
set_property(TEST HeatTransfer.BP4.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.BP4.Dump.MxN
)
