#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_hdf5Writer helloHDF5Writer.cpp)
  target_include_directories(hello_hdf5Writer PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_hdf5Writer ${MPI_C_LIBRARIES})
else()
  add_executable(hello_hdf5Writer helloHDF5Writer_nompi.cpp)
endif()
target_link_libraries(hello_hdf5Writer adios2)

if(ADIOS_BUILD_TESTING)
  add_test(NAME Example::hello::hdf5::write COMMAND hello_hdf5Writer)
endif()
