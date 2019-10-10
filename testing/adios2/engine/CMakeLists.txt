#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(bp)
add_subdirectory(skeleton)
add_subdirectory(inline)
add_subdirectory(null)
add_subdirectory(nullcore)

if(ADIOS2_HAVE_HDF5)
  add_subdirectory(hdf5)
endif()

if(ADIOS2_HAVE_DataMan)
  add_subdirectory(dataman)
endif()

if(ADIOS2_HAVE_Table)
  add_subdirectory(table)
endif()

if(ADIOS2_HAVE_SSC)
  add_subdirectory(ssc)
endif()

if(ADIOS2_HAVE_SST)
  add_subdirectory(sst)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(common)
  add_subdirectory(insitumpi)
endif()

add_subdirectory(staging-common)
