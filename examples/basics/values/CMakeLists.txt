#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(values_write values_write.cpp)

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  target_include_directories(values_write PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(values_write ${MPI_C_LIBRARIES})

endif()

target_link_libraries(values_write adios2)

