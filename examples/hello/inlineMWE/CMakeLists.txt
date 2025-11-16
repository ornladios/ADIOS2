#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2HelloInlineMWEExample)

set(maybe_standalone_project OFF)

if(NOT TARGET adios2_core)
  set(maybe_standalone_project ON)
  set(_components CXX)

  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

add_executable(adios2_hello_inlineMWE inlineMWE.cpp)
target_link_libraries(adios2_hello_inlineMWE adios2::cxx)
if (maybe_standalone_project)
install(TARGETS adios2_hello_inlineMWE RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
