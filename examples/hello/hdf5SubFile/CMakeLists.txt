#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(HelloHDF5SubFileExample)

if(NOT TARGET adios2_core)
  set(_components CXX)

  find_package(MPI COMPONENTS C)
  if(MPI_FOUND)
    # Workaround for various MPI implementations forcing the link of C++ bindings
    add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)

    list(APPEND _components MPI)
  endif()

  find_package(HDF5 QUIET COMPONENTS C)

  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

if(ADIOS2_HAVE_MPI AND ADIOS2_HAVE_HDF5 AND HDF5_VERSION VERSION_GREATER_EQUAL 1.14)
  add_executable(adios2_hello_hdf5Subfile hdf5SubFile.cpp)
  target_link_libraries(adios2_hello_hdf5Subfile adios2::cxx11_mpi MPI::MPI_CXX)
  install(TARGETS adios2_hello_hdf5Subfile RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

