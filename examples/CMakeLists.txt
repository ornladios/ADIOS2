#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(basics)
add_subdirectory(hello)
add_subdirectory(query)
add_subdirectory(useCases)

if(ADIOS2_HAVE_MPI)
  add_subdirectory(heatTransfer)
  if(ADIOS2_HAVE_HDF5)
    if(NOT (HDF5_VERSION VERSION_LESS 1.11))
       add_subdirectory(h5vol)	   
    endif()
  endif()
endif()

if(ADIOS2_BUILD_EXAMPLES_EXPERIMENTAL)
  add_subdirectory(experimental)
endif()
