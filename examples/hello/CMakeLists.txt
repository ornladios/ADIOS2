#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(bpAttributeWriter)
add_subdirectory(bpFlushWriter)

if(ADIOS2_HAVE_MPI)
  add_subdirectory(bpFWriteCRead)
endif()

add_subdirectory(bpReader)
add_subdirectory(bpThreadWrite)
add_subdirectory(bpTimeWriter)
add_subdirectory(bpWriter)

if(ADIOS2_HAVE_CUDA OR ADIOS2_HAVE_Kokkos_CUDA)
  add_subdirectory(bpWriteReadCuda)
endif()

find_package(hip QUIET)
if(ADIOS2_HAVE_Kokkos_HIP OR hip_FOUND)
  add_subdirectory(bpWriteReadHip)
endif()

if(ADIOS2_HAVE_Kokkos)
  add_subdirectory(bpWriteReadKokkos)
endif()

if(ADIOS2_HAVE_DataMan)
  add_subdirectory(datamanReader)
  add_subdirectory(datamanWriter)
endif()

if(ADIOS2_HAVE_DataSpaces)
  add_subdirectory(dataspacesReader)
  add_subdirectory(dataspacesWriter)
endif()

if(ADIOS2_HAVE_HDF5)
  add_subdirectory(hdf5Writer)
  add_subdirectory(hdf5Reader)
  if(HDF5_VERSION VERSION_GREATER_EQUAL 1.14)
    add_subdirectory(hdf5SubFile)
  endif()
endif()

add_subdirectory(helloWorld)

if(ADIOS2_HAVE_Fortran)
  add_subdirectory(inlineFWriteCppRead)
endif()

add_subdirectory(inlineMWE)
add_subdirectory(inlineReaderWriter)
add_subdirectory(skeleton)

if(ADIOS2_HAVE_SST)
  add_subdirectory(sstReader)
  add_subdirectory(sstWriter)
endif()
