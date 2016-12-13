#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.7)

# Fail immediately if not using an out-of-source build
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported.  Please create a build directory "
    "separate from the source directory")
endif()

project(ADIOS VERSION 2.0.0)

#------------------------------------------------------------------------------#
# Some boilerplate to setup nice output directories
#------------------------------------------------------------------------------#

include(GNUInstallDirs)

list(INSERT CMAKE_MODULE_PATH 0 "${ADIOS_SOURCE_DIR}/cmake")
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${ADIOS_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${ADIOS_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${ADIOS_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()

#------------------------------------------------------------------------------#
# Top level options
#------------------------------------------------------------------------------#

# Force C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CMakeDependentOption)

# Setup shared library / -fPIC stuff
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
cmake_dependent_option(BUILD_SHARED_LIBS
  "Whether or not to build shared libraries" ON
  "SHARED_LIBS_SUPPORTED" OFF)
if(SHARED_LIBS_SUPPORTED)
  cmake_dependent_option(ADIOS_ENABLE_PIC
    "Build with Position Independent Code" ON
    "NOT BUILD_SHARED_LIBS" ON)
endif()
if(ADIOS_ENABLE_PIC)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# MPI
option(ADIOS_USE_MPI "Enable the MPI version of ADIOS" ON)
if(ADIOS_USE_MPI)
  find_package(MPI COMPONENTS C)
  if(NOT MPI_FOUND)
    message(WARNING "Unable to find MPI.  Disabling ADIOS_USE_MPI")
    set(ADIOS_USE_MPI OFF CACHE BOOL "Enable the MPI version of ADIOS" FORCE)
  endif()
endif()

#------------------------------------------------------------------------------#
# Third party libraries
#------------------------------------------------------------------------------#
include(CTest)
add_subdirectory(thirdparty)

#------------------------------------------------------------------------------#
# Main library source
#------------------------------------------------------------------------------#
add_subdirectory(source)

#------------------------------------------------------------------------------#
# Installation
#------------------------------------------------------------------------------#
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)
#------------------------------------------------------------------------------#
# Examples
#------------------------------------------------------------------------------#
option(ADIOS_BUILD_EXAMPLES "Build ADIOS examples" ON)
if(ADIOS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

#------------------------------------------------------------------------------#
# Testing
#------------------------------------------------------------------------------#
# We have to wait until after the library is defined to enable testing
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(testing)
endif()
