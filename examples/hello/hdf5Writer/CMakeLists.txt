#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

add_executable(hello_hdf5Writer_nompi helloHDF5Writer_nompi.cpp)
message("    HDF5:   ${HDF5_INCLUDE_DIRS}")
include_directories(${HDF5_INCLUDE_DIRS})
target_link_libraries(hello_hdf5Writer_nompi adios2_nompi)

if(ADIOS_BUILD_TESTING)
  #add_test(
  #  NAME Example::hello::hdf5Writer_nompi
  #  COMMAND hello_hdf5Writer_nompi
  #)
endif()

if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C REQUIRED)

  add_executable(hello_hdf5Writer helloHDF5Writer.cpp)
  target_link_libraries(hello_hdf5Writer adios2)
  target_include_directories(hello_hdf5Writer PRIVATE ${MPI_C_INCLUDE_PATH})
  target_link_libraries(hello_hdf5Writer adios2 ${MPI_C_LIBRARIES})

  if(ADIOS_BUILD_TESTING)
    add_test(NAME Example::hello::hdf5Writer COMMAND hello_hdf5Writer)
  endif()
endif()

