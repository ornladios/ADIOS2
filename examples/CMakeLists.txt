#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(basics)
add_subdirectory(hello)
add_subdirectory(useCases)

if(ADIOS2_HAVE_MPI)
  #if(MPIEXEC_MAX_NUMPROCS GREATER 3)
    add_subdirectory(heatTransfer)
  #endif()
endif()

if(ADIOS2_BUILD_EXAMPLES_EXPERIMENTAL)
  add_subdirectory(experimental)
endif()
