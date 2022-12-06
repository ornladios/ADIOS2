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

find_package(Kokkos QUIET)
if(Kokkos_FOUND)
  add_subdirectory(kokkos)
endif()

if(ADIOS2_HAVE_MPI)
  add_subdirectory(heatTransfer)
endif()

if(ADIOS2_BUILD_EXAMPLES_EXPERIMENTAL)
  add_subdirectory(experimental)
endif()

if(ADIOS2_HAVE_CUDA)
  add_subdirectory(cuda)
endif()
