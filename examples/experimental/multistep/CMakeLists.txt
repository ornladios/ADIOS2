#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(writer_multistep writer_multistep.cpp)
add_executable(reader_stepping reader_stepping.cpp)
add_executable(reader_allsteps reader_allsteps.cpp)

foreach(tgt IN ITEMS writer_multistep reader_stepping reader_allsteps)
  target_link_libraries(${tgt} adios2)

  if(ADIOS2_HAVE_MPI)
    target_include_directories(${tgt} PRIVATE ${MPI_C_INCLUDE_PATH})
    target_link_libraries(${tgt} ${MPI_C_LIBRARIES})
  endif()
endforeach()
