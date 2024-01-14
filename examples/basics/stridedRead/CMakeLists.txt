#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2BasicsStridedReadExample)

if(NOT TARGET adios2_core)
  set(_components CXX)
  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

add_executable(adios2_basics_stridedRead1D stridedRead1D.cpp)
target_link_libraries(adios2_basics_stridedRead1D adios2::cxx11 adios2_core)
install(TARGETS adios2_basics_variablesShapes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(adios2_basics_stridedRead2D stridedRead2D.cpp)
target_link_libraries(adios2_basics_stridedRead2D adios2::cxx11 adios2_core)
install(TARGETS adios2_basics_variablesShapes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

