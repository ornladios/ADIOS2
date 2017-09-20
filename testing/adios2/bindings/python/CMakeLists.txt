#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT ADIOS2_HAVE_MPI)
    python_add_test(NAME PythonBPWrite SCRIPT TestBPWriteTypes_nompi.py)
endif()

if(ADIOS2_HAVE_MPI)
    set(test_parameters
      EXEC_WRAPPER
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS})
    python_add_test(NAME PythonBPWrite ${test_parameters} SCRIPT TestBPWriteTypes.py)
endif()
