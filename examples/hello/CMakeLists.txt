#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(bpWriter)
add_subdirectory(bpReader)
add_subdirectory(bpTimeWriter)
add_subdirectory(bpFlushWriter)
add_subdirectory(bpAttributeWriter)
add_subdirectory(callback)
add_subdirectory(skeleton)

if(ADIOS2_HAVE_ADIOS1)
  add_subdirectory(adios1Writer)
endif()

if(ADIOS2_HAVE_DataMan)
  add_subdirectory(datamanReader)
  add_subdirectory(datamanWriter)
endif()

if(ADIOS2_HAVE_SST)
  add_subdirectory(sstReader)
  add_subdirectory(sstWriter)
endif()

if(ADIOS2_HAVE_HDF5)
  add_subdirectory(hdf5Writer)
endif()

if(ADIOS2_HAVE_BZip2)
  add_subdirectory(bpBZip2Wrapper)
endif()

if(ADIOS2_HAVE_ZFP)
  add_subdirectory(bpZfpWrapper)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(insituMPI)
  add_subdirectory(bpFWriteCRead)
endif()
