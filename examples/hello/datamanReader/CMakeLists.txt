#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_datamanReader_nompi helloDataManReader_nompi.cpp)
target_link_libraries(hello_datamanReader_nompi adios2_nompi)

if(ADIOS_BUILD_TESTING)
  add_test(
    NAME Example::hello::datamanReader_nompi
    COMMAND hello_datamanReader_nompi
  )
endif()

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_datamanReader helloDataManReader.cpp)
  target_link_libraries(hello_datamanReader adios2)
  target_include_directories(hello_datamanReader PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_datamanReader adios2 ${MPI_C_LIBRARIES})

  if(ADIOS_BUILD_TESTING)
    add_test(NAME Example::hello::datamanReader COMMAND hello_datamanReader)
  endif()
endif()
  
