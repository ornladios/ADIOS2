#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(basics)
if (ADIOS2_HAVE_Derived)
  add_subdirectory(derived)
endif()
add_subdirectory(hello)
add_subdirectory(plugins)
add_subdirectory(simulations)
add_subdirectory(useCases)
