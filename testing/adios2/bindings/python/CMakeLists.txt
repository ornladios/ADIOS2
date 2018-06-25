#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT ADIOS2_HAVE_MPI)
    python_add_test(NAME PythonBPWriteReadTypes SCRIPT TestBPWriteReadTypes_nompi.py)
endif()

if(ADIOS2_HAVE_MPI)
    set(test_parameters
      EXEC_WRAPPER
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS})
    python_add_test(NAME PythonBPWriteReadTypes ${test_parameters} 
    				SCRIPT TestBPWriteReadTypes.py)
    python_add_test(NAME PythonBPWriteReadHighLevelAPI ${test_parameters} 
    				SCRIPT TestBPWriteTypesHighLevelAPI.py)
    python_add_test(NAME PythonBPReadMultisteps ${test_parameters} 
    				SCRIPT TestBPReadMultisteps.py)
endif()
