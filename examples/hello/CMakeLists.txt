#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(bpWriter)
add_subdirectory(bpReader)
add_subdirectory(bpTimeWriter)
add_subdirectory(bpFlushWriter)
add_subdirectory(bpAttributeWriter)
add_subdirectory(inlineReaderWriter)
add_subdirectory(skeleton)

if(ADIOS2_HAVE_DataMan)
  add_subdirectory(datamanReader)
  add_subdirectory(datamanWriter)
endif()

if(ADIOS2_HAVE_DataSpaces)
  add_subdirectory(dataspacesReader)
  add_subdirectory(dataspacesWriter)
endif()

if(ADIOS2_HAVE_SST)
  add_subdirectory(sstReader)
  add_subdirectory(sstWriter)
endif()

if(ADIOS2_HAVE_HDF5)
  add_subdirectory(hdf5Writer)
  add_subdirectory(hdf5Reader)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(bpFWriteCRead)
endif()

if(ADIOS2_HAVE_Fortran)
    add_subdirectory(inlineFWriteCppRead)
endif()
