#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.5)

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

# Default to a debug build if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Force C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CMakeDependentOption)

# Setup shared library / -fPIC stuff
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
cmake_dependent_option(ADIOS_BUILD_SHARED_LIBS
  "Whether or not to build shared libraries" ON
  "SHARED_LIBS_SUPPORTED" OFF)
if(SHARED_LIBS_SUPPORTED)
  cmake_dependent_option(ADIOS_ENABLE_PIC
    "Build with Position Independent Code" ON
    "NOT ADIOS_BUILD_SHARED_LIBS" ON)
endif()
set(BUILD_SHARED_LIBS ${ADIOS_BUILD_SHARED_LIBS})

if(ADIOS_ENABLE_PIC)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

option(ADIOS_USE_MPI "Enable the MPI version of ADIOS" OFF)
if(ADIOS_USE_MPI)
  # Workaround for OpenMPI forcing the link of C++ bindings
  add_definitions(-DOMPI_SKIP_MPICXX)
endif()
option(ADIOS_USE_BZip2 "Enable support for BZip2 transforms" OFF)
option(ADIOS_USE_ZFP "Enable support for ZFP transforms" OFF)
option(ADIOS_USE_ADIOS1 "Enable support for the ADIOS 1 engine" OFF)
option(ADIOS_USE_HDF5 "Enable support for the HDF5 engine" OFF)

# DataMan is not a user-settable option.  It will always be enabled if the
# platform supports it.
set(ADIOS_USE_DataMan ${SHARED_LIBS_SUPPORTED})

include(GenerateADIOSConfig)
GenerateADIOSConfig(MPI ZFP BZip2 ADIOS1 HDF5 DataMan)
install(
  FILES ${ADIOS_BINARY_DIR}/adios2/ADIOSConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2
)

#------------------------------------------------------------------------------#
# Third party libraries
#------------------------------------------------------------------------------#
option(ADIOS_BUILD_TESTING "Build ADIOS tests" ON)
set(BUILD_TESTING ${ADIOS_BUILD_TESTING})
mark_as_advanced(BUILD_TESTING)
include(CTest)
add_subdirectory(thirdparty)

#------------------------------------------------------------------------------#
# Main library source
#------------------------------------------------------------------------------#
add_subdirectory(source)

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

#------------------------------------------------------------------------------#
# Configuration summary
#------------------------------------------------------------------------------#
message("")
message("ADIOS2 build configuration:")
message("  ADIOS Version: ${ADIOS_VERSION}")
message("  C++ Compiler : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} ${CMAKE_CXX_COMPILER_WRAPPER}")
message("    ${CMAKE_CXX_COMPILER}")
message("")
message("  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message("  Features:")
if(BUILD_SHARED_LIBS)
  message("    Library Type: shared")
else()
  message("    Library Type: static")
endif()
message("    Build Type:   ${CMAKE_BUILD_TYPE}")
message("    Testing: ${BUILD_TESTING}")
message("    MPI:     ${ADIOS_USE_MPI}")
message("    BZip2:   ${ADIOS_USE_BZip2}")
message("    ZFP:     ${ADIOS_USE_ZFP}")
message("    ADIOS1:  ${ADIOS_USE_ADIOS1}")
message("    DataMan: ${ADIOS_USE_DataMan}")
message("      ZeroMQ:  ${ADIOS_USE_DataMan_ZeroMQ}")
message("      ZFP:     ${ADIOS_USE_DataMan_ZFP}")
message("    HDF5:    ${ADIOS_USE_HDF5}")
message("")
