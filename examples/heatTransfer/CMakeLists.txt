#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(write)
add_subdirectory(read)
add_subdirectory(read_fileonly)

# BP File tests
add_test(NAME HeatTransfer.BPFile.Write.M
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_write_adios2>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.M 2 2 10 10 10 10
)

# BPFile MxM
add_test(NAME HeatTransfer.BPFile.Read.MxM
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.M.bp HeatTransfer.BPFile.Read.MxM 2 2
)
set_property(TEST HeatTransfer.BPFile.Read.MxM
  PROPERTY DEPENDS HeatTransfer.BPFile.Write.M
)

add_test(NAME HeatTransfer.BPFile.Dump.MxM
  COMMAND bpls -lad HeatTransfer.BPFile.Read.MxM.bp
)
set_property(TEST HeatTransfer.BPFile.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.BPFile.Read.MxM
)

# BPFile Mx1
add_test(NAME HeatTransfer.BPFile.Read.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
    $<TARGET_FILE:heatTransfer_read>
    ${CMAKE_CURRENT_SOURCE_DIR}/heat_bpfile.xml
    HeatTransfer.BPFile.Write.M.bp HeatTransfer.BPFile.Read.Mx1 1 1
)
set_property(TEST HeatTransfer.BPFile.Read.Mx1
  PROPERTY DEPENDS HeatTransfer.BPFile.Write.M
)

add_test(NAME HeatTransfer.BPFile.Dump.Mx1
  COMMAND bpls -lad HeatTransfer.BPFile.Read.Mx1.bp
)
set_property(TEST HeatTransfer.BPFile.Dump.Mx1
  PROPERTY DEPENDS HeatTransfer.BPFile.Read.Mx1
)


# SST MxM test
add_test(NAME HeatTransfer.SST.MxM
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.M 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.M.bp HeatTransfer.SST.Read.MxM 2 2
)
add_test(NAME HeatTransfer.SST.Dump.MxM
  COMMAND bpls -lad HeatTransfer.SST.Read.MxM.bp
)
set_property(TEST HeatTransfer.SST.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.SST.Read.MxM
)

# SST Mx1 test
add_test(NAME HeatTransfer.SST.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.M 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.SST.Write.M.bp HeatTransfer.SST.Read.Mx1 1 1
)
add_test(NAME HeatTransfer.SST.Dump.Mx1
  COMMAND bpls -lad HeatTransfer.SST.Read.Mx1.bp
)
set_property(TEST HeatTransfer.SST.Dump.Mx1
  PROPERTY DEPENDS HeatTransfer.SST.Read.Mx1
)


# InsituMPI MxM test
add_test(NAME HeatTransfer.InsituMPI.MxM
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.InsituMPI.Write.M 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_sst.xml
      HeatTransfer.InsituMPI.Write.M.bp HeatTransfer.InsituMPI.Read.MxM 2 2
)
add_test(NAME HeatTransfer.InsituMPI.Dump.MxM
  COMMAND bpls -lad HeatTransfer.InsituMPI.Read.MxM.bp
)
set_property(TEST HeatTransfer.InsituMPI.Dump.MxM
  PROPERTY DEPENDS HeatTransfer.InsituMPI.Read.MxM
)

# InsituMPI Mx1 test
add_test(NAME HeatTransfer.InsituMPI.Mx1
  COMMAND ${MPIEXEC_EXECUTABLE}
    ${MPIEXEC_NUMPROC_FLAG} 4
      $<TARGET_FILE:heatTransfer_write_adios2>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_insitumpi.xml
      HeatTransfer.InsituMPI.Write.M 2 2 10 10 10 10
  : ${MPIEXEC_NUMPROC_FLAG} 1
      $<TARGET_FILE:heatTransfer_read>
      ${CMAKE_CURRENT_SOURCE_DIR}/heat_insitumpi.xml
      HeatTransfer.InsituMPI.Write.M.bp HeatTransfer.InsituMPI.Read.Mx1 1 1
)
add_test(NAME HeatTransfer.InsituMPI.Dump.Mx1
  COMMAND bpls -lad HeatTransfer.InsituMPI.Read.Mx1.bp
)
set_property(TEST HeatTransfer.InsituMPI.Dump.Mx1
  PROPERTY DEPENDS HeatTransfer.InsituMPI.Read.Mx1
)


