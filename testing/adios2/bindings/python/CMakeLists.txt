#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(NOT ADIOS2_HAVE_MPI)
    python_add_test(PythonBPWrite TestBPWriteTypes_nompi.py)
endif()

if(ADIOS2_HAVE_MPI)
    python_add_test(PythonBPWrite TestBPWriteTypes.py)
endif()