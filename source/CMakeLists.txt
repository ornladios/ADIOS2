#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(adios2)
add_subdirectory(utils)

# HDF5 VOL requires 1.13+
if(ADIOS2_HAVE_HDF5)
  if(HDF5_VERSION VERSION_LESS 1.13)
    set(ADIOS2_HAVE_HDF5_VOL OFF CACHE INTERNAL "")
  else()
    set(ADIOS2_HAVE_HDF5_VOL ON CACHE INTERNAL "")
  endif()
else()
  set(ADIOS2_HAVE_HDF5_VOL OFF CACHE INTERNAL "")
endif()
if(ADIOS2_HAVE_HDF5_VOL)
  add_subdirectory(h5vol)
endif()
