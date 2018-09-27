#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

include(GoogleTest)

if(ADIOS2_HAVE_MPI)
  set(MPIEXEC_COMMAND
    ${MPIEXEC_EXECUTABLE} 
        ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
  )
endif()
add_subdirectory(adios2)
add_subdirectory(utils)
