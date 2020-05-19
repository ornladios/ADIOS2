#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(heatTransfer_inline_adios2
  main.cpp
  InlineIO.cpp
  ../write/HeatTransfer.cpp
  ../write/Settings.cpp
)
target_link_libraries(heatTransfer_inline_adios2
  adios2::cxx11_mpi MPI::MPI_C ${CMAKE_THREAD_LIBS_INIT}
)
