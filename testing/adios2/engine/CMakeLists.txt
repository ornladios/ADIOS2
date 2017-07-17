#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

# We currently require ADIOS1 to test ADIOS v2 bp functionality since the read
# API is not available yet
add_subdirectory(bp)

if(ADIOS2_HAVE_ADIOS1)
  add_subdirectory(adios1)
endif()

if(ADIOS2_HAVE_HDF5)
  add_subdirectory(hdf5)
endif()

if(ADIOS2_HAVE_DataMan)
  add_subdirectory(dataman)
endif()
