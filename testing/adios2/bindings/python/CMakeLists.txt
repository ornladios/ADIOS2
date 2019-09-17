#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

function(add_python_mpi_test testname)
  python_add_test(NAME Bindings.Python.${testname} SCRIPT Test${testname}.py
    EXEC_WRAPPER ${MPIEXEC_COMMAND}
  )
  set_tests_properties(Bindings.Python.${testname} PROPERTIES
    PROCESSORS ${NUM_TEST_PROCS}
  )
endfunction()

python_add_test(NAME Bindings.Python.HighLevelAPI SCRIPT TestHighLevelAPI.py) 

if(NOT ADIOS2_HAVE_MPI)
  python_add_test(NAME Bindings.Python.BPWriteReadTypes
    SCRIPT TestBPWriteReadTypes_nompi.py
  )
endif()

if(ADIOS2_HAVE_MPI)
  add_python_mpi_test(BPWriteReadTypes)
  add_python_mpi_test(BPWriteTypesHighLevelAPI)
  add_python_mpi_test(BPWriteTypesHighLevelAPILocal)
  add_python_mpi_test(BPReadMultisteps)
  add_python_mpi_test(BPWriteRead2D)
  add_python_mpi_test(BPChangingShapeHighLevelAPI)
  add_python_mpi_test(NullEngine)

  # Currently hangs in H5Fclose for unknown reasons
  #if(ADIOS2_HAVE_HDF5)
  #  add_python_mpi_test(BPWriteTypesHighLevelAPI_HDF5)
  #endif()
    
  if(ADIOS2_HAVE_ZFP)
    add_python_mpi_test(BPZfpHighLevelAPI)
  endif()
    
  if(ADIOS2_HAVE_PNG)
    add_python_mpi_test(BPPNGHighLevelAPI)
  endif()
endif()

