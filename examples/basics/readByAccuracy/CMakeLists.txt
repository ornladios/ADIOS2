#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2BasicsReadByAccuracyExample)

if(NOT TARGET adios2_core)
  set(_components CXX)
  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

add_executable(adios2_basics_readByAccuracy2D readByAccuracy2D.cpp)
target_link_libraries(adios2_basics_readByAccuracy2D adios2::cxx11 adios2_core)
install(TARGETS adios2_basics_readByAccuracy2D RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

