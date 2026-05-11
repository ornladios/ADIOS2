# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2MHDOrszagTangExample)

set(maybe_standalone_project OFF)
if(NOT TARGET adios2_core)
  set(maybe_standalone_project ON)
  set(_components CXX)

  find_package(MPI COMPONENTS C)
  if(MPI_FOUND)
    # Workaround for various MPI implementations forcing the link of C++ bindings
    add_definitions(-DOMPI_SKIP_MPICXX -DMPICH_SKIP_MPICXX)

    list(APPEND _components MPI)
  endif()

  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

set(mhd_orszag_tang_sources
  src/main.cpp
  src/mhd2d.cpp
  src/adios_writer.cpp
)

add_executable(mhd_orszag_tang ${mhd_orszag_tang_sources})
target_include_directories(mhd_orszag_tang
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(ADIOS2_HAVE_MPI)
  find_package(MPI COMPONENTS CXX REQUIRED)
  target_compile_definitions(mhd_orszag_tang PRIVATE OT_USE_MPI=1)
  target_link_libraries(mhd_orszag_tang PRIVATE adios2::cxx_mpi MPI::MPI_CXX)
else()
  target_link_libraries(mhd_orszag_tang PRIVATE adios2::cxx)
endif()

if (maybe_standalone_project)
  install(TARGETS mhd_orszag_tang RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
