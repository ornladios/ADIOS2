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
include(ADIOSFunctions)

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
cmake_dependent_option(ADIOS_ENABLE_PIC
  "Build with Position Independent Code" ON
  "SHARED_LIBS_SUPPORTED" OFF)
set(BUILD_SHARED_LIBS ${ADIOS_BUILD_SHARED_LIBS})

if(ADIOS_ENABLE_PIC)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

adios_option(BZip2   "Enable support for BZip2 transforms" AUTO)
adios_option(ZFP     "Enable support for ZFP transforms" AUTO)
adios_option(MPI     "Enable support for MPI" AUTO)
adios_option(DataMan "Enable support for DataMan" AUTO)
adios_option(ZeroMQ  "Enable support for ZeroMQ" AUTO)
adios_option(HDF5    "Enable support for the HDF5 engine" AUTO)
adios_option(ADIOS1  "Enable support for the ADIOS 1 engine" AUTO)
adios_option(Python  "Enable support for Python bindings" AUTO)
include(${ADIOS_SOURCE_DIR}/cmake/DetectOptions.cmake)

if(ADIOS_HAVE_MPI)
  # Workaround for OpenMPI forcing the link of C++ bindings
  add_definitions(-DOMPI_SKIP_MPICXX)
endif()

GenerateADIOSConfig(MPI ZFP BZip2 ADIOS1 HDF5 DataMan Python)
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
# Language bindings
#------------------------------------------------------------------------------#
add_subdirectory(bindings)

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
message("    Build Options:")
foreach(opt BZip2 ZFP MPI DataMan ZeroMQ HDF5 ADIOS1 Python)
  message_pad("      ${opt}" 13 label)
  if(${ADIOS_HAVE_${opt}})
    message("${label}: ON")
  else()
    message("${label}: OFF")
  endif()
endforeach()
message("")
