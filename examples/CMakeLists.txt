#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_subdirectory(basics)
add_subdirectory(hello)
add_subdirectory(query)
add_subdirectory(useCases)
add_subdirectory(inlineMWE)
add_subdirectory(plugins)
add_subdirectory(fides)

if(ADIOS2_HAVE_MPI)
  add_subdirectory(heatTransfer)
endif()

if(ADIOS2_BUILD_EXAMPLES_EXPERIMENTAL)
  add_subdirectory(experimental)
endif()

if(ADIOS2_HAVE_CUDA OR ADIOS2_HAVE_Kokkos_CUDA)
  add_subdirectory(cuda)
endif()


if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_HDF5)
  if(HDF5_VERSION VERSION_GREATER_EQUAL 1.14)
    add_subdirectory(h5subfile)
  endif()
endif()
