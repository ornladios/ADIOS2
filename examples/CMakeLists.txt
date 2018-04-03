#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(basics)
add_subdirectory(hello)

if(ADIOS2_HAVE_MPI)
  add_subdirectory(heatTransfer)
endif()

add_subdirectory(plugins)
add_subdirectory(highLevelAPI)

if(ADIOS2_BUILD_EXAMPLES_EXPERIMENTAL)
  add_subdirectory(experimental)
endif()
