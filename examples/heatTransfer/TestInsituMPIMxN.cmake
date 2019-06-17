#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_test(NAME HeatTransfer.InsituMPI.MxN
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_insitumpi.xml
      HeatTransfer.InsituMPI.Write.MxN 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 3
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_insitumpi.xml
      HeatTransfer.InsituMPI.Write.MxN.bp HeatTransfer.InsituMPI.Read.MxN 1 3
)

add_test(NAME HeatTransfer.InsituMPI.Dump.MxN
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-d 
    -DINPUT_FILE=HeatTransfer.InsituMPI.Read.MxN.bp
    -DOUTPUT_FILE=HeatTransfer.InsituMPI.Dump.MxN.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)
set_property(TEST HeatTransfer.InsituMPI.Dump.MxN
  PROPERTY DEPENDS HeatTransfer.InsituMPI.Read.MxN
)

add_test(NAME HeatTransfer.InsituMPI.Validate.MxN
  COMMAND ${DIFF_EXECUTABLE} -u
    ${CMAKE_CURRENT_SOURCE_DIR}/HeatTransfer.Dump.txt
    ${CMAKE_CURRENT_BINARY_DIR}/HeatTransfer.InsituMPI.Dump.MxN.txt
)
set_property(TEST HeatTransfer.InsituMPI.Validate.MxN
  PROPERTY DEPENDS HeatTransfer.InsituMPI.Dump.MxN
)
