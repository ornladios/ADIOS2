#---------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#---------------------------------------------------------------------#

# SetupTestPipeline
include(ADIOSFunctions)

set(mpicmd ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG})

#------------------------------------------
#  Pipe2 BP Write
#------------------------------------------
add_test(NAME Utils.IOTest.Pipe2.BP.Write
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
    $<TARGET_FILE:adios_iotest>
      -a 1 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2.txt
      -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-BP.xml -d 2 1 --strong-scaling
)
set_tests_properties(Utils.IOTest.Pipe2.BP.Write PROPERTIES PROCESSORS 2)

add_test(NAME Utils.IOTest.Pipe2.BP.Write.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-laD
    -DINPUT_FILE=pipe2_write.bp
    -DOUTPUT_FILE=IOTest.Pipe2.BP.Write.bpls.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME Utils.IOTest.Pipe2.BP.Write.Validate
  COMMAND ${DIFF_COMMAND} -uw
    ${CMAKE_CURRENT_SOURCE_DIR}/IOTest.Pipe2.BP.Write.bpls.txt
    IOTest.Pipe2.BP.Write.bpls.txt
)

SetupTestPipeline(
  Utils.IOTest.Pipe2.BP
  "Write;Write.Dump;Write.Validate"
  TRUE
)

#------------------------------------------
#  Pipe2 BP Read
#------------------------------------------
add_test(NAME Utils.IOTest.Pipe2.BP.Read
  COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
    $<TARGET_FILE:adios_iotest>
      -a 2 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2.txt
      -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-BP.xml -d 2 1 --strong-scaling
)
set_tests_properties(Utils.IOTest.Pipe2.BP.Read PROPERTIES PROCESSORS 2)

add_test(NAME Utils.IOTest.Pipe2.BP.Read.Dump
  COMMAND ${CMAKE_COMMAND}
    -DARG1=-laD
    -DINPUT_FILE=pipe2_read.bp
    -DOUTPUT_FILE=IOTest.Pipe2.BP.Read.bpls.txt
    -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
)

add_test(NAME Utils.IOTest.Pipe2.BP.Read.Validate
  COMMAND ${DIFF_COMMAND} -uw
    ${CMAKE_CURRENT_SOURCE_DIR}/IOTest.Pipe2.BP.Read.bpls.txt
    IOTest.Pipe2.BP.Read.bpls.txt
)

SetupTestPipeline(
  Utils.IOTest.Pipe2.BP
  "Write;Read;Read.Dump;Read.Validate"
  FALSE
)

if(ADIOS2_HAVE_HDF5)
  #------------------------------------------
  #  Pipe2 HDF5 Write
  #------------------------------------------
  add_test(NAME Utils.IOTest.Pipe2.HDF5.Write
    COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
      $<TARGET_FILE:adios_iotest>
        -a 1 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-HDF5.txt
        -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-HDF5.xml -d 2 1 --strong-scaling
  )
  set_tests_properties(Utils.IOTest.Pipe2.HDF5.Write PROPERTIES PROCESSORS 2)

  add_test(NAME Utils.IOTest.Pipe2.HDF5.Write.Dump
    COMMAND ${CMAKE_COMMAND}
      -DARG1=-laD
      -DINPUT_FILE=pipe2_write.h5
      -DOUTPUT_FILE=IOTest.Pipe2.HDF5.Write.bpls.txt
      -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
  )

  add_test(NAME Utils.IOTest.Pipe2.HDF5.Write.Validate
    COMMAND ${DIFF_COMMAND} -uw
      ${CMAKE_CURRENT_SOURCE_DIR}/IOTest.Pipe2.HDF5.Write.bpls.txt
      IOTest.Pipe2.HDF5.Write.bpls.txt
  )

  SetupTestPipeline(
    Utils.IOTest.Pipe2.HDF5
    "Write;Write.Dump;Write.Validate"
    TRUE
  )

  #------------------------------------------
  #  Pipe2 HDF5 Read
  #------------------------------------------
  add_test(NAME Utils.IOTest.Pipe2.HDF5.Read
    COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2
      $<TARGET_FILE:adios_iotest>
        -a 2 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-HDF5.txt
        -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-HDF5.xml -d 2 1 --strong-scaling
  )
  set_tests_properties(Utils.IOTest.Pipe2.HDF5.Read PROPERTIES PROCESSORS 2)

  add_test(NAME Utils.IOTest.Pipe2.HDF5.Read.Dump
    COMMAND ${CMAKE_COMMAND}
      -DARG1=-laD
      -DINPUT_FILE=pipe2_read.h5
      -DOUTPUT_FILE=IOTest.Pipe2.HDF5.Read.bpls.txt
      -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
  )

  add_test(NAME Utils.IOTest.Pipe2.HDF5.Read.Validate
    COMMAND ${DIFF_COMMAND} -uw
      ${CMAKE_CURRENT_SOURCE_DIR}/IOTest.Pipe2.HDF5.Read.bpls.txt
      IOTest.Pipe2.HDF5.Read.bpls.txt
  )

  SetupTestPipeline(
    Utils.IOTest.Pipe2.HDF5
    "Write;Read;Read.Dump;Read.Validate"
    FALSE
  )
endif()

if(MPIEXEC_MAX_NUMPROCS GREATER 1) 
  #------------------------------------------
  #  Pipe2 InsituMPI version 
  #------------------------------------------
  add_test(NAME Utils.IOTest.Pipe2.InSituMPI
    COMMAND ${MPIEXEC_EXECUTABLE}
      ${MPIEXEC_NUMPROC_FLAG} 1 $<TARGET_FILE:adios_iotest>
        -a 1 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2.txt
        -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-InSituMPI.xml -d 1 1
        --strong-scaling
      :
      ${MPIEXEC_NUMPROC_FLAG} 1 $<TARGET_FILE:adios_iotest>
        -a 2 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2.txt
        -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-InSituMPI.xml -d 1 1
        --strong-scaling 
  )
  set_tests_properties(Utils.IOTest.Pipe2.InSituMPI PROPERTIES PROCESSORS 2)

  add_test(NAME Utils.IOTest.Pipe2.InSituMPI.Dump
    COMMAND ${CMAKE_COMMAND}
      -DARG1=-laD
      -DINPUT_FILE=pipe2_read.bp
      -DOUTPUT_FILE=IOTest.Pipe2.InSituMPI.bpls.txt
      -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
  )

  add_test(NAME Utils.IOTest.Pipe2.InSituMPI.Validate
    COMMAND ${DIFF_COMMAND} -uw
      ${CMAKE_CURRENT_SOURCE_DIR}/IOTest.Pipe2.InSituMPI.bpls.txt
      IOTest.Pipe2.InSituMPI.bpls.txt
  )

  SetupTestPipeline(
    Utils.IOTest.Pipe2.InSituMPI
    ";Dump;Validate"
    TRUE
  )

  #------------------------------------------
  #  Pipe2 SST version 
  #------------------------------------------
  if(ADIOS2_HAVE_SST)
    add_test(NAME Utils.IOTest.Pipe2.SST
      COMMAND ${MPIEXEC_EXECUTABLE}
        ${MPIEXEC_NUMPROC_FLAG} 1 $<TARGET_FILE:adios_iotest>
          -a 1 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2.txt
          -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-SST.xml -d 1 1 --strong-scaling
        :
        ${MPIEXEC_NUMPROC_FLAG} 1 $<TARGET_FILE:adios_iotest>
          -a 2 -c ${CMAKE_CURRENT_SOURCE_DIR}/pipe2.txt
          -x ${CMAKE_CURRENT_SOURCE_DIR}/pipe2-SST.xml -d 1 1 --strong-scaling 
    )
	
    add_test(NAME Utils.IOTest.Pipe2.SST.Dump
      COMMAND ${CMAKE_COMMAND}
        -DARG1=-laD
        -DINPUT_FILE=pipe2_read.bp
        -DOUTPUT_FILE=IOTest.Pipe2.SST.bpls.txt
        -P "${PROJECT_BINARY_DIR}/$<CONFIG>/bpls.cmake"
    )

    add_test(NAME Utils.IOTest.Pipe2.SST.Validate
      COMMAND ${DIFF_COMMAND} -uw
        ${CMAKE_CURRENT_SOURCE_DIR}/IOTest.Pipe2.SST.bpls.txt
        IOTest.Pipe2.SST.bpls.txt
    )

    SetupTestPipeline(
      Utils.IOTest.Pipe2.SST
      ";Dump;Validate"
      TRUE
    )
  endif()
else()
  message(WARNING "Not enough (2) MPI procs for insitu tests with adios_iotest")
endif() 
