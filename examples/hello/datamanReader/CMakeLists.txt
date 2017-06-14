#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_HAVE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_datamanReader helloDataManReader.cpp)
  target_include_directories(hello_datamanReader PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_datamanReader ${MPI_C_LIBRARIES})
else()
  add_executable(hello_datamanReader helloDataManReader_nompi.cpp)
endif()
target_link_libraries(hello_datamanReader adios2)
