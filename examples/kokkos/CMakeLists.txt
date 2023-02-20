#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(KokkosWriteRead kokkosWriteRead.cpp)
target_link_libraries(KokkosWriteRead PUBLIC adios2::cxx11 Kokkos::kokkos)

if(Kokkos_ENABLE_CUDA)
  set_property(TARGET KokkosWriteRead PROPERTY CUDA_STANDARD 17)
  set_property(SOURCE kokkosWriteRead.cpp PROPERTY LANGUAGE CUDA)
  set_property(SOURCE kokkosWriteRead.cpp APPEND PROPERTY COMPILE_FLAGS "--extended-lambda")
endif()
