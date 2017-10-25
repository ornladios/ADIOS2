#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

foreach(tgt IN ITEMS writer_multistep reader_stepping reader_allsteps)
  add_executable(${tgt} ${tgt}.cpp)
  target_link_libraries(${tgt} adios2)

  if(ADIOS2_HAVE_MPI)
    target_link_libraries(${tgt} MPI::MPI_C)
  endif()
endforeach()
