#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)
  
  add_executable(hello_datamanWriter helloDataManWriter.cpp)
  target_link_libraries(hello_datamanWriter adios2)
  target_include_directories(hello_datamanWriter PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_datamanWriter adios2 ${MPI_C_LIBRARIES})

  if(ADIOS_BUILD_TESTING)
    add_test(NAME Example::hello::datamanWriter COMMAND hello_datamanWriter)
  endif()
else()
  add_executable(hello_datamanWriter_nompi helloDataManWriter_nompi.cpp)
  target_link_libraries(hello_datamanWriter_nompi adios2)

  if(ADIOS_BUILD_TESTING)
    add_test(
      NAME Example::hello::datamanWriter_nompi
      COMMAND hello_datamanWriter_nompi
    )
  endif()
endif()

