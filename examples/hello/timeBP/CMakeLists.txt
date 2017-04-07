#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_timeBPWriter_nompi timeBPWriter_nompi.cpp)
target_link_libraries(hello_timeBPWriter_nompi adios2_nompi)

if(ADIOS_BUILD_TESTING)
  add_test(
    NAME Example::hello::timeBPWriter_nompi
    COMMAND hello_timeBPWriter_nompi
  )
endif()

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  add_executable(hello_timeBPWriter timeBPWriter.cpp)
  target_link_libraries(hello_timeBPWriter adios2)
  target_include_directories(hello_timeBPWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_timeBPWriter adios2 ${MPI_C_LIBRARIES})

  if(ADIOS_BUILD_TESTING)
    add_test(NAME Example::hello::timeBPWriter COMMAND hello_timeBPWriter)
  endif()
endif()
