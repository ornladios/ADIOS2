#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.InsituMPI.MxM
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_insitumpi.xml
      HeatTransfer.InsituMPI.Write.MxM 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_insitumpi.xml
      HeatTransfer.InsituMPI.Write.MxM.bp HeatTransfer.InsituMPI.Read.MxM 2 2
)

add_test(NAME HeatTransfer.InsituMPI.Dump.MxM
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=HeatTransfer.InsituMPI.Read.MxM.bp
    -DOUTPUT_FILE=HeatTransfer.InsituMPI.Dump.MxM.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.InsituMPI.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.InsituMPI.Read.MxM
)

add_test(NAME HeatTransfer.InsituMPI.Validate.MxM
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.InsituMPI.Dump.MxM.txt
)
set_property(TEST HeatTransfer.InsituMPI.Validate.MxM
  PROPERTY DEPENDS HeatTransfer.InsituMPI.Dump.MxM
)
