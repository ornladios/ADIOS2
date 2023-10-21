#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2HelloBPWriteReadHipExample)

if(NOT TARGET adios2_core)
  set(_components CXX)

  find_package(hip QUIET)
  if(hip_FOUND)
    enable_language(HIP)
  endif()

  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

if(ADIOS2_HAVE_Kokkos_HIP OR hip_FOUND)
  add_executable(adios2_hello_bpWriteReadHip bpWriteReadHip.cpp)
  target_link_libraries(adios2_hello_bpWriteReadHip adios2::cxx11 hip::device)
  set_source_files_properties(bpWriteReadHip.cpp PROPERTIES LANGUAGE HIP)
  install(TARGETS adios2_hello_bpWriteReadHip RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
