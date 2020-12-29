#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(adios2)
add_subdirectory(utils)

# HDF5 VOL requires a matching adios configuration
if(ADIOS2_HAVE_HDF5 AND HDF5_VERSION VERSION_GREATER_EQUAL 1.11 AND
   (NOT HDF5_IS_PARALLEL OR (HDF5_IS_PARALLEL AND ADIOS2_USE_MPI)))
  set(ADIOS2_HAVE_HDF5_VOL ON CACHE INTERNAL "" FORCE)
  add_subdirectory(h5vol)
else()
  set(ADIOS2_HAVE_HDF5_VOL OFF CACHE INTERNAL "" FORCE)
endif()
