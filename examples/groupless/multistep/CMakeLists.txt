#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(writer_multistep writer_multistep.cpp)
  target_include_directories(writer_multistep PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(writer_multistep ${MPI_C_LIBRARIES})

  add_executable(reader_stepping reader_stepping.cpp)
  target_include_directories(reader_stepping PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(reader_stepping ${MPI_C_LIBRARIES})

  add_executable(reader_allsteps reader_allsteps.cpp)
  target_include_directories(reader_allsteps PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(reader_allsteps ${MPI_C_LIBRARIES})

endif()
target_link_libraries(writer_multistep adios2)
target_link_libraries(reader_stepping adios2)
target_link_libraries(reader_allsteps adios2)

