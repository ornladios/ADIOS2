#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(Kokkos)

add_executable(KokkosWriteRead kokkosWriteRead.cpp)
target_link_libraries(KokkosWriteRead PUBLIC adios2::cxx11 Kokkos::kokkos)
