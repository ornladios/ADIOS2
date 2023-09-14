#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(globalArrayND)
add_subdirectory(joinedArray)
add_subdirectory(localArray)
if(ADIOS2_HAVE_DATAMAN)
  add_subdirectory(queryWorker)
endif()
add_subdirectory(values)
