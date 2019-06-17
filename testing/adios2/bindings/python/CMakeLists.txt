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
    python_add_test(NAME PythonBPWriteReadHighLevelAPILocal ${test_parameters} 
                    SCRIPT TestBPWriteTypesHighLevelAPILocal.py)
    python_add_test(NAME PythonBPReadMultisteps ${test_parameters} 
                    SCRIPT TestBPReadMultisteps.py)
    python_add_test(NAME PythonBPWriteRead2D ${test_parameters} 
                    SCRIPT TestBPWriteRead2D.py)
    python_add_test(NAME PythonBPChangingShapeHighLevelAPI ${test_parameters} 
                    SCRIPT TestBPChangingShapeHighLevelAPI.py)
    python_add_test(NAME PythonNullEngine ${test_parameters} 
                    SCRIPT TestNullEngine.py)
    
    if (ADIOS2_HAVE_HDF5)
        python_add_test(NAME PythonBPWriteReadHighLevelAPI_HDF5 
                        SCRIPT TestBPWriteTypesHighLevelAPI_HDF5.py)
    endif(ADIOS2_HAVE_HDF5)
    
    if (ADIOS2_HAVE_ZFP)
        python_add_test(NAME PythonBPZfpHighLevelAPI ${test_parameters}
                        SCRIPT TestBPZfpHighLevelAPI.py)
    endif(ADIOS2_HAVE_ZFP)
    
    if (ADIOS2_HAVE_PNG)
        python_add_test(NAME PythonBPPNGHighLevelAPI ${test_parameters}
                        SCRIPT TestBPPNGHighLevelAPI.py)
    endif(ADIOS2_HAVE_PNG)
    
endif()

