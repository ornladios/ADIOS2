#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(manyvars)
add_subdirectory(query)

if(ADIOS2_BUILD_LARGE_TESTS)
  add_subdirectory(largevar)
endif()
